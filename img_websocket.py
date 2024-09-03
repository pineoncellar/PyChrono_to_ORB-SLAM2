# -*- encoding: utf-8 -*- #
"""                                                                      
      .o.         .oooooo.    oooooo     oooo 
     .888.       d8P'  `Y8b    `888.     .8'  
    .8"888.     888             `888.   .8'   
   .8' `888.    888              `888. .8'    
  .88ooo8888.   888     ooooo     `888.8'     
 .8'     `888.  `88.    .88'       `888'      
o88o     o8888o  `Y8bood8P'         `8'

@Author    :   pine
@Contact   :   pine@hydroroll.team
@Desc      :   检测文件夹变动，建立图片发送ws服务端
@Update    :   2024/7/4 xx:xx:xx
"""

import time
import asyncio
import cv2
import websockets
import numpy as np
import json
import re
from watchdog.events import *
from watchdog.observers import Observer
import threading

########################################## 参数定义 ##########################################

# 服务地址与端口
server_host = "0.0.0.0"
server_port = 29507

# chrono输出地址
chrono_output_path = "D:\\Anaconda3\\envs\\chrono\\Lib\\site-packages\\pychrono\\demos\\sensor\\SENSOR_OUTPUT"

# 程序内存最大储存图片数
max_img_count = 100

########################################## watchdog ############################################


# 看门狗操作类
class FileEventHandler(FileSystemEventHandler):
    def __init__(self):
        FileSystemEventHandler.__init__(self)

    # 文件移动
    def on_moved(self, event):
        timestamp = getTimeStamp()
        if event.is_directory:
            print(
                "[{2}] directory moved from {0} to {1}".format(
                    event.src_path, event.dest_path, timestamp
                )
            )
        else:
            print(
                "[{2}] file moved from {0} to {1}".format(
                    event.src_path, event.dest_path, timestamp
                )
            )

    # 文件创建
    def on_created(self, event):
        timestamp = getTimeStamp()
        if event.is_directory:
            print("[{1}] directory created:{0}".format(event.src_path, timestamp))
        else:
            print("[{1}] file created:{0}".format(event.src_path, timestamp))

    # 文件删除
    def on_deleted(self, event):
        timestamp = getTimeStamp()
        if event.is_directory:
            print("[{1}] directory deleted:{0}".format(event.src_path, timestamp))
        else:
            print("[{1}] file deleted:{0}".format(event.src_path, timestamp))

    # 文件更改
    # chrono运行时，每一张图片会出现2-3次更改
    def on_modified(self, event):
        # timestamp = getTimeStamp()
        # print("[{1}] file modified:{0}".format(event.src_path, timestamp))
        last_index = len(img_list.img_list) - 1
        file_index = int(re.search(r"(\d+).png", event.src_path).group(0)[:-4])
        if file_index - 2 == last_index:
            img = cv2.imread(f"{chrono_output_path}\cam\\frame_{file_index-1}.png")
            img_list.addImg(img, last_index + 1)
            # print(f"add img to {last_index+1}")
        # 将先前存入的图片释放掉
        if last_index >= max_img_count:
            img_list.img_list[last_index - max_img_count] = None
            # print(f"img {last_index - max_img_count} deleted.")

        """
        if last_index / 50 - int(last_index / 50) == 0:
            print(f"last_index:{last_index}, last_index / 50 - int(last_index / 50) = {last_index / 50 - int(last_index / 50)}")
            img_list.updateIndex()
            img_list.debugImg()
        """


# 文件系统监听函数
def watchdog_server(path):
    observer = Observer()
    observer.schedule(FileEventHandler(), path=path, recursive=False)
    observer.start()
    print(f"watching directory: {path}")
    try:
        while observer.is_alive():
            observer.join(1)
    except KeyboardInterrupt:
        observer.stop()
        exit(1)
    observer.join()
    exit(1)


########################################## websocket ############################################

"""
msg:
{
    "action": str, # init / start video stream / end video stream
    "data": {}
}
res:
{
    "action": str, #sample
    "stat": bool,
    "data":{},
    "msg": # stat为false时不为空
}
"""


# 视频流发送任务函数
async def camera_stream(websocket):
    print(f"create task.")
    while True:
        await img_list.sendImg(websocket)
        await asyncio.sleep(0.05)


# WebSocket服务端处理函数
async def websocket_server(websocket, path):
    print("client connected.")
    video_stream_task = asyncio.create_task(camera_stream(websocket))
    video_stream_task.cancel()
    # print("task create success.")

    try:
        async for message in websocket:
            # 消息处理
            print(f"get post:{message}")
            msg = json.loads(message)

            action = msg["action"]
            data = msg["data"]
            if action == "start video stream":
                # 开始发送img
                print("Start sending img.")
                img_list.updateIndex()  # 更新一下索引
                if video_stream_task.done():
                    # 创建一个发送视频流的任务
                    video_stream_task.cancel()  # 取消之前可能存在的任务
                    video_stream_task = asyncio.create_task(camera_stream(websocket))

            elif action == "end video stream":
                # 停止发送img
                print("Stopped sending img.")
                video_stream_task.cancel()
                try:
                    await video_stream_task
                except asyncio.CancelledError:
                    pass  # 忽略取消任务时引发的异常

            else:
                # 仅打印接收到的其他消息
                pass
                # print(f"Received: {message}")

    except websockets.exceptions.ConnectionClosed:
        print("WebSocket connection closed")

    except KeyboardInterrupt:
        exit(1)

    finally:
        # 确保无论如何都取消video_stream_task
        if not video_stream_task.done():
            video_stream_task.cancel()
        try:
            await video_stream_task
        except asyncio.CancelledError:
            pass  # 忽略在取消任务时引发的异常


# 启动WebSocket服务器
def start_websocket_server():
    start_server = websockets.serve(websocket_server, server_host, server_port)
    print(f"server starting at {server_host}:{server_port}...")
    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()
    print(f"ws server started.")


########################################## 图片操作类与其他函数 ############################################


# 图片存读操作类
class ImgHandler:
    def __init__(self):
        self.img_list = []
        self.last_img = -1
        self.max_index = -1

    async def sendImg(self, websocket):
        img_index = self.last_img
        stat, img = self.getImg(img_index)

        if stat:
            print(f"send img {img_index}")
            self.last_img = img_index + 1
            buffer = cv2.imencode(".jpg", img)[1]
            await websocket.send(buffer.tobytes())
            # await asyncio.sleep(0.05)
        else:
            pass

    def addImg(self, img, index):
        if index == self.max_index + 1:
            self.img_list.append(img)
            self.max_index += 1

    def updateIndex(self):
        if self.last_img == len(self.img_list) - 1:
            pass
        else:
            print(f"last_img update from {self.last_img} to {len(self.img_list)-1}")
            self.last_img = len(self.img_list) - 1

    def getImg(self, index):
        if index <= self.max_index:
            return True, self.img_list[index]
        else:
            # print(f"error: img {index} not found")
            return False, None


# 获取格式化的时间
def getTimeStamp() -> str:
    return time.ctime()[11:-5]


########################################## 主流程 ############################################

if __name__ == "__main__":
    img_list = ImgHandler()
    timestamp = getTimeStamp()

    # 运行watchdog，需在运行ws服务之前
    watchdog_thread = threading.Thread(
        target=watchdog_server, args=(chrono_output_path + "\\cam",)
    )
    watchdog_thread.start()

    # 运行ws server
    start_websocket_server()

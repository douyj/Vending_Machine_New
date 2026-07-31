#!/usr/bin/env python3
"""
无人贩卖机 TCP Server 测试端。

用途：
1. 模拟 PC Qt 后台监听 9000 端口。
2. 接收售货机发来的 4字节长度头 + JSON正文。
3. 支持手动发送测试命令给售货机。
"""

import json
import socket
import struct
import threading
import time


HOST = "127.0.0.1"
PORT = 9000
MAX_JSON_LEN = 64 * 1024

_seq = 1
_seq_lock = threading.Lock()
_conn = None
_conn_lock = threading.Lock()
_running = True


def next_seq():
    """生成请求序号，用来匹配售货机 response。"""
    global _seq

    with _seq_lock:
        value = _seq
        _seq += 1
        return value


def recv_all(sock, size):
    """循环读取指定字节数，解决 TCP 半包问题。"""
    data = bytearray()

    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            return None
        data.extend(chunk)

    return bytes(data)


def recv_json(sock):
    """读取一条完整 JSON 消息：4字节长度头 + JSON正文。"""
    len_buf = recv_all(sock, 4)
    if len_buf is None:
        return None

    json_len = struct.unpack("!I", len_buf)[0]
    if json_len <= 0 or json_len > MAX_JSON_LEN:
        raise ValueError(f"invalid json length: {json_len}")

    body = recv_all(sock, json_len)
    if body is None:
        return None

    return body.decode("utf-8")


def send_json(sock, obj):
    """发送一条 JSON 消息：4字节长度头 + JSON正文。"""
    body = json.dumps(obj, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    header = struct.pack("!I", len(body))
    sock.sendall(header + body)


def build_request(cmd, data=None):
    """构造发给售货机的 request 消息。"""
    return {
        "version": 1,
        "type": "request",
        "seq": next_seq(),
        "cmd": cmd,
        "device_id": "VM001",
        "data": data or {},
    }


def print_json(prefix, text):
    """格式化打印 JSON，方便观察协议内容。"""
    try:
        obj = json.loads(text)
        formatted = json.dumps(obj, ensure_ascii=False, indent=2)
        print(f"\n{prefix}\n{formatted}\n> ", end="", flush=True)
    except json.JSONDecodeError:
        print(f"\n{prefix}\n{text}\n> ", end="", flush=True)


def receiver_loop(sock):
    """持续接收售货机发来的心跳、事件和响应。"""
    global _running

    while _running:
        try:
            text = recv_json(sock)
            if text is None:
                print("\n售货机连接已断开")
                break
            print_json("收到售货机消息:", text)
        except Exception as exc:
            print(f"\n接收失败: {exc}")
            break

    _running = False


def send_request(cmd, data=None):
    """向当前已连接的售货机发送 request。"""
    with _conn_lock:
        sock = _conn

    if sock is None:
        print("当前没有售货机连接")
        return

    request = build_request(cmd, data)
    send_json(sock, request)
    print(f"已发送: {cmd}, seq={request['seq']}")


def input_loop():
    """命令行交互，用来手动测试后台下发命令。"""
    global _running

    help_text = (
        "\n可用命令:\n"
        "  status                 查询设备状态\n"
        "  products               查询商品列表\n"
        "  setstock <id> <stock>  修改库存，例如 setstock 1 20\n"
        "  open                   远程开门\n"
        "  close                  远程关门\n"
        "  quit                   退出测试端\n"
    )
    print(help_text)

    while _running:
        try:
            line = input("> ").strip()
        except EOFError:
            break

        if not line:
            continue

        parts = line.split()
        cmd = parts[0]

        try:
            if cmd == "status":
                send_request("device.status")
            elif cmd == "products":
                send_request("product.list")
            elif cmd == "setstock":
                if len(parts) != 3:
                    print("用法: setstock <product_id> <stock>")
                    continue
                send_request(
                    "product.update_stock",
                    {"product_id": int(parts[1]), "stock": int(parts[2])},
                )
            elif cmd == "open":
                send_request("door.open")
            elif cmd == "close":
                send_request("door.close")
            elif cmd == "quit":
                _running = False
                break
            else:
                print(help_text)
        except Exception as exc:
            print(f"发送失败: {exc}")

    _running = False


def main():
    """启动测试服务器，等待售货机连接。"""
    global _conn

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)

        print(f"TCP Server 测试端已启动，监听 {HOST}:{PORT}")
        print("请启动售货机程序，等待它主动连接...")

        sock, addr = server.accept()
        with sock:
            with _conn_lock:
                _conn = sock

            print(f"售货机已连接: {addr[0]}:{addr[1]}")

            receiver = threading.Thread(target=receiver_loop, args=(sock,), daemon=True)
            receiver.start()

            input_loop()

            with _conn_lock:
                _conn = None

        print("TCP Server 测试端已退出")


if __name__ == "__main__":
    main()

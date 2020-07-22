# coding=utf-8
import frida
import sys
import time

package = 'com.jlxy.demo'

def on_message(message, data):
    if message['type'] == 'send':
        libname = message['payload']['libname']
        funcname = message['payload']['funcname']
        type = message['payload']['type']
        offset = message['payload']['offset']
        Addr = message['payload']['Addr']
        Arr = message['payload']['Arr']
        trueAddr = message['payload']['trueAddr']
        trueArr = message['payload']['trueArr']
        Jmpto = message['payload']['Jmpto']

        out = type + "   " + libname + "   " + funcname + " + " + hex(offset) + "   " + Jmpto + "\n" \
              + "mem - " + Addr + ": " + ''.join('0x%02x ' % c for c in Arr) + "\n" \
              + "file- " + trueAddr + ": " + ''.join('0x%02x ' % c for c in trueArr) + "\n"
        print(out)
    else:
        print(message)


if __name__ == "__main__":
    # sys.argv[0] Main.py的路径
    if len(sys.argv) == 2:
        package = sys.argv[1]

    device = frida.get_usb_device()
    pid = device.spawn([package])
    if pid == -1:
        print("package %s not found!" % package)
        exit(2)

    device.resume(pid)
    time.sleep(1)

    session = device.attach(pid)
    if session is None:
        print("package %s, pid: %d, attach failed!" % package % pid)
        exit(3)

    with open("Run.js", "r", encoding='utf8') as f:
        js = f.read()

    script = session.create_script(js)
    script.on("message", on_message)
    script.load()
    sys.stdin.read()

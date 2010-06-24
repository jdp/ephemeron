import zmq

data_cmds = ['SET']

def main():
    entry = raw_input().split()
    ctx = zmq.Context(1)
    s = ctx.socket(zmq.REQ)
    s.connect("tcp://127.0.0.1:5555")
    if entry[0].upper() in data_cmds:
        command = " ".join(entry[0:-1])
        data = entry[-1]
        s.send(command + "\n" + data)
    else:
        command = " ".join(entry)
        s.send(command + "\n")
    response = s.recv()
    print response
    
if __name__ == '__main__':
    main()
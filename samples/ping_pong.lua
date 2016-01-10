require 'simul'

client = simul.app {
    name  = 'client',
    source = [[
native @nohold _DBG();
input void OS_START;
output/input (char[]&&)=>char[]&& PING_PONG;

await OS_START;

loop i do
    await 5s;

    var char[] str = [].."Ping "..['0'+i];
    _DBG("send: %s\n", &&str);

    var int err;
    var char[]&&? ret;
    (err,ret) = (request PING_PONG => &&str);
    _DBG("recv: %s\n", ret!);
end
]],
}

server = simul.app {
    name  = 'server',
    source = [[
native @nohold _DBG();

interface Global with
    var int i;
end
var int i = 0;

input/output (char[]&& str)=>char[]&& PING_PONG do
    _DBG("recv: %s\n", str);
    loop i in 3 do
        _DBG("thinking...\n");
        await 1s;
    end
    _DBG("returning...\n");
    var char[] ret = [].."Pong "..['0'+global:i];
    global:i = global:i + 1;
    return &&ret;
end
await FOREVER;
]],
}

simul.link(client,'OUT_PING_PONG_REQUEST', server,'IN_PING_PONG_REQUEST')
simul.link(client,'OUT_PING_PONG_CANCEL',  server,'IN_PING_PONG_CANCEL')
simul.link(server,'OUT_PING_PONG_RETURN',  client,'IN_PING_PONG_RETURN')

simul.shell()

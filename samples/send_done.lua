require 'simul'

client = simul.app {
    name  = 'client',
    source = [[
input void OS_START;
output/input (char[]&&)=>void SEND_DONE;
await OS_START;
_DBG("sending...\n");
var int err;
var char[] str = [].."Hello World!";
err = (request SEND_DONE => &&str);
_DBG("done %d!\n", err);
escape 0;
]],
}

server = simul.app {
    name  = 'server',
    source = [[
native @nohold _DBG();
input/output (char[]&& v)=>void SEND_DONE do
    _DBG("received %s\n", v);
    loop i in 10 do
        _DBG("thinking...\n");
        await 1s;
    end
    _DBG("returning...\n");
    return;
end
await FOREVER;
]],
}

simul.link(client,'OUT_SEND_DONE_REQUEST', server,'IN_SEND_DONE_REQUEST')
simul.link(client,'OUT_SEND_DONE_CANCEL',  server,'IN_SEND_DONE_CANCEL')
simul.link(server,'OUT_SEND_DONE_RETURN',  client,'IN_SEND_DONE_RETURN')

simul.shell()

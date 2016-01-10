require 'simul'

client = simul.app {
    name  = 'client',
    source = [[
input void OS_START;
output/input (void)=>char SERIAL_CHAR;

await OS_START;
loop i in 3 do
    await 1s;
    _DBG("sending request...\n");

    var int err;
    var char? v;
    (err,v) = request SERIAL_CHAR;
    _DBG("done [%d]: %c!\n", err, v!);
end
escape 0;
]],
}

server = simul.app {
    name  = 'server',
    source = [[
native @nohold _DBG();
native do
    int IDX = -1;
    char VEC[] = { 'a','b','c' };
end
input/output (void)=>char SERIAL_CHAR do
    _DBG("received request\n");
    loop i in 3 do
        _DBG("thinking...\n");
        await 1s;
    end
    _DBG("returning...\n");
    _IDX = _IDX + 1;
    return _VEC[_IDX];
end
await FOREVER;
]],
}

simul.link(client,'OUT_SERIAL_CHAR_REQUEST', server,'IN_SERIAL_CHAR_REQUEST')
simul.link(client,'OUT_SERIAL_CHAR_CANCEL',  server,'IN_SERIAL_CHAR_CANCEL')
simul.link(server,'OUT_SERIAL_CHAR_RETURN',  client,'IN_SERIAL_CHAR_RETURN')

simul.shell()

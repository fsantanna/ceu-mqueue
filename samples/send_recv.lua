require 'simul'

recv = simul.app {
    name  = 'recv',
    source = [[
native _DBG();
input int A;
loop do
    var int v = await A;
    _DBG("Received: %d\n", v);
end
]],
}

send = simul.app {
    name  = 'send',
    source = [[
native _DBG();
output int B;
var int v = 1;
loop do
    emit B => v;
    _DBG("Sent: %d\n", v);
    await 1s;
    v = v + 1;
end
]],
}

simul.link(send,'OUT_B', recv,'IN_A')

simul.shell()

del *.obj
copy ../fixed/common.cpp ./common.cpp
wpp386 -3s -zp4 -zk0 -ox -d0 -D__TOWNS__ common.cpp -fo=.obj
wpp386 -3s -zp4 -zk0 -ox -d0 -D__TOWNS__ main.cpp -fo=.obj
wpp386 -3s -zp4 -zk0 -ox -d0 -D__TOWNS__ render.cpp -fo=.obj
wpp386 -3s -zp4 -zk0 -ox -d0 -D__TOWNS__ sound.cpp -fo=.obj
wlink system pharlap runtime CALLBUFS=8192 option STACK=65536 name openlara file{ common.obj main.obj render.obj sound.obj .\lib\tbios.lib .\lib\fmcfrb.lib .\lib\snd.lib}

import { Actor, AttributeId, CANCEL, chat, command, MinecraftPacketIds, nethook, NetworkIdentifier } from "bdsx";
import { netevent } from "bdsx";
import { BlockPos } from "bdsx/bds/blockpos";
import { TextPacket } from "bdsx/bds/packets";
import { Minecraft } from "bdsx/bds/server";
import { int32_t, uint32_t } from "bdsx/nativetype";
import { SimpleForm } from "./api/form";
import Scoreboard from "./api/scoreboard";

interface Pos2{ x:int32_t, z:int32_t }

const system = server.registerSystem(0,0);

let board = new Scoreboard("island");
board.getScore(`LastPos/x`,(x)=>{
    if(x===0) board.setScore(`LastPos/x`,1);
});
board.getScore(`LastPos/z`,(x)=>{
    if(x===0) board.setScore(`LastPos/z`,1);
});


nethook.before(MinecraftPacketIds.Text).on((pk,ni)=>{
    if(pk.message.startsWith('>')){
        try {
            let tpk = TextPacket.create();
            tpk.message = JSON.stringify(eval(pk.message.substr(1)));
            tpk.sendTo(ni);
            tpk.dispose()
        } catch (error) {
            let tpk = TextPacket.create();
            tpk.message = JSON.stringify(error);
            tpk.sendTo(ni);
            tpk.dispose()
        }
        return CANCEL;
    }
})

system.listenForEvent("minecraft:entity_created",(ev)=>{
    if(ev.data.entity.__identifier__!=="minecraft:player") return;
    let name = system.getComponent(ev.data.entity,"minecraft:nameable")!.data.name;
    // setTimeout(() => {
    //     make(name,{x:-1,z:-1}).then(val=>{
    //         console.log(val);
    //     }).then(()=>{
    //         make(name,{x:-2,z:-1}).then(val=>{
    //             console.log(val);
    //         });
    //     });
    // }, 10000);
    
});

const sleep = (ms:number) => {
    return new Promise(resolve=>{
        setTimeout(resolve,ms)
    })
}

command.hook.on((cmd,name)=>{
    if(cmd === "/sb"){
        system.executeCommand(`gamemode s "${name}"`,()=>{});
        board.getScore(`${name}/x`,(x)=>{
            board.getScore(`${name}/z`,(z)=>{
                if(x===0&&z===0){
                    board.getScore(`LastPos/x`,(lx)=>{
                        board.getScore(`LastPos/z`,(lz)=>{
                            if(lx===lz){
                                lx++;
                                lz=0;
                            }
                            lz++;
                            board.setScore(`LastPos/x`,lx);
                            board.setScore(`LastPos/z`,lz);
                            board.setScore(`${name}/x`,lx);
                            board.setScore(`${name}/z`,lz);

                            system.executeCommand(`tp "${name}" ${lx*16+8} 16 ${lz*16+8}`,()=>{});
                            console.log(`Making Skyblock... (${lx},${lz})`)
                            make({x:lx,z:lz}).then(()=>{
                                system.executeCommand(`setblock ${lx*16+8} 15 ${lz*16+8} dirt`,()=>{});

                                system.executeCommand(`setblock ${lx*16+7} 14 ${lz*16+8} dirt`,()=>{});
                                system.executeCommand(`setblock ${lx*16+9} 14 ${lz*16+8} dirt`,()=>{});
                                system.executeCommand(`setblock ${lx*16+8} 14 ${lz*16+7} dirt`,()=>{});
                                system.executeCommand(`setblock ${lx*16+8} 14 ${lz*16+9} dirt`,()=>{});
                            
                            
                                system.executeCommand(`tp "${name}" ${lx*16+8} 16 ${lz*16+8}`,()=>{});
                            });
                        });
                    });
                }else{
                    system.executeCommand(`tp "${name}" ${x*16+8} 16 ${z*16+8}`,()=>{});
                }
            });
        });
        return 0;
    }
})

async function make(_pos:Pos2){
    let pos = {x:_pos.x*16,z:_pos.z*16};
    for(let x=pos.x;x<=pos.x+16;x++){
        for(let z=pos.z;z<=pos.z+16;z++){
            if(x==pos.x || x==pos.x+16 ||z==pos.z || z==pos.z+16){
                system.executeCommand(`fill ${x} 1 ${z} ${x} 255 ${z} bedrock`,(val)=>{console.log(val)});
            }else{
                system.executeCommand(`fill ${x} 255 ${z} ${x} 255 ${z} barrier`,(val)=>{console.log(val)})
            }
        }
        await sleep(50);
    }
}




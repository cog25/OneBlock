import { NetworkIdentifier } from "bdsx";
import { MinecraftPacketIds } from "bdsx";
import { SetTitlePacket, TextPacket } from "bdsx/bds/packets";
import { command } from "bdsx/command";
import { int32_t } from "bdsx/nativetype";
import { nethook } from "bdsx/nethook";
import { connected } from "node:process";
import { ChatColor } from "./api/ChatColor";
import { CustomForm, sendModalForm, SimpleForm } from "./api/form";
import Scoreboard from "./api/scoreboard";
import { data } from "./GenData";

const system = server.registerSystem(0,0);

let board = new Scoreboard("island");
let blockCounter = new Scoreboard("BlockCounter");
let lv = new Scoreboard("Level");
interface Pos2{ x:int32_t, z:int32_t }
let tmp = new Map<string,VectorXYZ>();

system.listenForEvent("minecraft:player_destroyed_block",(e)=>{
    let pos = e.data.block_position;
    let name = system.getComponent(e.data.player,"minecraft:nameable")!.data.name;
    if(tmp.has(name)){
        let main = tmp.get(name)!
        if(pos.x===main.x&&pos.y===main.y&&pos.z===main.z){
            genBlock(pos,name);
        }
    }else{
        board.getScore(`${name}/x`,(x)=>{
            board.getScore(`${name}/z`,(z)=>{
                let main = {x:x*16+8,y:15,z:z*16+8}
                tmp.set(name,main);
                if(pos.x===main.x&&pos.y===main.y&&pos.z===main.z){
                    genBlock(pos,name);
                }
            });
        });
        
    }
});

let NiByName = new Map<string,NetworkIdentifier>();
nethook.after(MinecraftPacketIds.Login).on((pk,ni)=>{
    NiByName.set(pk.connreq.cert.getId(),ni);
    console.log((new Date()).toDateString());
});
nethook.after(MinecraftPacketIds.Disconnect).on((pk,ni)=>{
    NiByName.forEach((val,key)=>{
        if(val===ni){
            NiByName.delete(key);
        }
    })
});

command.hook.on((cmd,name)=>{
    if(cmd==="/gen"){
        let form = new SimpleForm("Choose Generator","");
        lv.getScore(`${name}/max`,(max)=>{
            data.forEach((val,idx)=>{
                if(idx>max){
                    form.addButton(`${ChatColor.DarkGray}[UNLOCKED] ${val.name} (${val.requiredBlock})`)
                }else{
                    form.addButton(`${val.name} (${val.requiredBlock})`);
                }
            });
            sendModalForm(NiByName.get(name)!,form,(e)=>{
                if(e.formData===null) return;
                if(e.formData>max){
                    let pk = TextPacket.create();
                    pk.message = `${ChatColor.DarkRed}This Generator is UNLOCKED`;
                    pk.sendTo(NiByName.get(name)!);
                    pk.dispose();
                }else{
                    lv.setScore(name,e.formData);
                    let pk = TextPacket.create();
                    pk.message = `${ChatColor.Green}Generator Changed to ${data[e.formData].name}`;
                    pk.sendTo(NiByName.get(name)!);
                    pk.dispose();


                    board.getScore(`${name}/x`,(x)=>{
                        board.getScore(`${name}/z`,(z)=>{
                            let main = {x:x*16+8,y:15,z:z*16+8}
                            genBlock(main,name);
                        });
                    });
                }
            });
            return 0;
        });
    }
});

function genBlock(pos:VectorXYZ,name:string){
    blockCounter.getScore(name,(cnt)=>{
        lv.getScore(`${name}/max`,(max)=>{
            try {
                if(data[max+1].requiredBlock<=cnt){
                    system.executeCommand(`/playsound random.orb "${name}"`,()=>{});
                    system.executeCommand(`/title "${name}" subtitle ${data[max+1].name}  `,()=>{});
                    system.executeCommand(`/title "${name}" title ${ChatColor.LightPurple}[${ChatColor.White} ${ChatColor.Green}New Generator Unlock! ${ChatColor.LightPurple}]${ChatColor.White}`,()=>{});
                    lv.setScore(`${name}/max`,max+1);
                }
            } catch (error) {
                
            }
            lv.getScore(`${name}`,(now)=>{
                let cnt= Math.random()*100;
                for(let i of data[now].blocks){
                    cnt=cnt-i.precent;
                    if(cnt<=0){
                        system.executeCommand(`setblock ${pos.x} ${pos.y} ${pos.z} ${i.name} ${i.id!==undefined ? i.id:0}`,()=>{});
                        blockCounter.addScore(name,1,()=>{
                            system.executeCommand(`/titleraw "${name}" actionbar {"rawtext":[{"text":"§e캔 블록 수:§f "},{"score":{"name":"*","objective":"BlockCounter"}},{"text":"개"}]}`,()=>{});
                        });
                        break; 
                    }
                }
                system.executeCommand(`setblock ${pos.x} ${pos.y} ${pos.z} ${data[now].blocks[data[now].blocks.length].name} ${data[now].blocks[data[now].blocks.length].id!==undefined ? data[now].blocks[data[now].blocks.length].id:0}`,()=>{});
                        blockCounter.addScore(name,1,()=>{
                            system.executeCommand(`/titleraw "${name}" actionbar {"rawtext":[{"text":"§e캔 블록 수:§f "},{"score":{"name":"*","objective":"BlockCounter"}},{"text":"개"}]}`,()=>{});
                });
            });
        });
    });    
}
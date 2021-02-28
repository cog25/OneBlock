const system = server.registerSystem(0,0);

export default class Scoreboard{
    obj:string
    constructor(obj:string,displayName:string=obj){
        system.executeCommand(`scoreboard objectives add "${obj}" dummy "${displayName}"`,()=>{}); 
        this.obj = obj;
    }

    getScore(player:string,cb:(score:number)=>any){
        system.executeCommand(`scoreboard players add "${player}" "${this.obj}" 0`,(res)=>{
            let txt = res.data.statusMessage;
            txt = /\((.*?)\)/gi.exec(txt)![1];
            txt = txt.replace("now ","");
            cb(Number(txt));
        });
    }

    setScore(player:string,score:number,cb?:(statusCode:number)=>any){
        system.executeCommand(`scoreboard players set "${player}" "${this.obj}" ${score}`,(res)=>{
            if(cb!= undefined) cb!(res.data.statusCode);
        });
    }

    addScore(player:string,score:number,cb?:(statusCode:number)=>any){
        system.executeCommand(`scoreboard players add "${player}" "${this.obj}" ${score}`,(res)=>{
            if(cb!= undefined) cb!(res.data.statusCode);
        });
    }
    
    resetScore(player:string,cb?:(statusCode:number)=>any){
        system.executeCommand(`scoreboard players reset "${player}" "${this.obj}"`,(res)=>{
            if(cb!= undefined) cb!(res.data.statusCode);
        });
    }
}
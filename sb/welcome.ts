import { MinecraftPacketIds } from "bdsx";
import { NetworkIdentifier } from "bdsx";
import { nethook } from "bdsx/nethook";
import { ChatColor } from "./api/ChatColor";
import { sendModalForm, SimpleForm } from "./api/form";


let NameByNi = new Map<NetworkIdentifier,string>();

nethook.after(MinecraftPacketIds.Login).on((pk,ni)=>{
    let name = pk.connreq.cert.getId();
    NameByNi.set(ni,name);
});


nethook.after(MinecraftPacketIds.SetLocalPlayerAsInitialized).on((pk,ni)=>{
    let name = NameByNi.get(ni)!;
    const txt = ChatColor.White +
    `${ChatColor.Green}${name}${ChatColor.White}님 환영합니다! \n\n` +
    ` - ${ChatColor.LightPurple}/sb${ChatColor.White}라고 입력하시면 본인의 섬으로 이동합니다.\n` +
    ` (최초 이동시 섬 이동에 시간이 걸릴 수 있습니다.)\n\n` +
    ` - 특정 갯수만큼 블록을 캘 경우, 다음 테마가 해금됩니다 \n\n` +
    ` - 더 ${ChatColor.Green}많은 기능${ChatColor.White}이 추가 될 예정입니다! 개발자에게 아이디어를 제공해주세요!\n\n` +
    `Developer: ${ChatColor.LightPurple}Gridroot(Cog25)${ChatColor.DarkGray} \n\n`;

    let form = new SimpleForm(`Welcome to ${ChatColor.Green}Oneblock${ChatColor.White} Server!`,txt);
    sendModalForm(ni,form,(data)=>{console.log(data)});

})
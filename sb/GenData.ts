import { ChatColor } from "./api/ChatColor";

interface Block {name:string,id?:number,precent:number};
interface LV{
    name:string,
    requiredBlock:number,
    blocks:Block[],
}
export const data:LV[] = [
    {
        name:`${ChatColor.Green}새로운 시작`,
        requiredBlock: 0, 
        blocks:[
            {
                name: "log",
                id: 0,
                precent: 8
            },
            {
                name: "log",
                id: 1,
                precent: 8
            },
            {
                name: "log",
                id: 2,
                precent: 8
            },
            {
                name: "log",
                id: 3,
                precent: 8
            },
            {
                name: "log2",
                id: 0,
                precent: 8
            },
            {
                name:"leaves",
                id:0,
                precent: 5
            },
            {
                name:"leaves",
                id:0,
                precent: 5
            },
            {
                name:"grass",
                precent: 20
            },
            {
                name:"dirt",
                precent:30
            },
        ]
    },
    {
        name: `${ChatColor.DarkGray}얕은 지하`,
        requiredBlock: 200,
        blocks:[
            {
                name: "stone",
                precent: 30,
            },
            {
                name: "stone", // 안산암
                id: 5,
                precent: 10,
            },
            {
                name: "stone", // 섬록암
                id: 3,
                precent: 10,
            },
            { 
                name: "cobblestone",
                precent: 30,
            },
            {
                name: "iron_ore",
                precent: 24,
            },
            {
                name: "coal_ore",
                precent: 26,
            },
        ]
    },
    {
        name: `${ChatColor.Yellow}모래 사막`,
        requiredBlock: 300,
        blocks:[
            {
                name: "sand",
                precent: 70,
            },
            {
                name: "water",
                precent: 5,
            },
            {
                name: "gravel",
                precent: 25,
            },
        ]
    },
    {
        name: `${ChatColor.Black}심층 지하`,
        requiredBlock: 500,
        blocks:[
            {
                name: "stone",
                precent: 15,
            },
            {
                name: "stone", // 안산암
                id: 5,
                precent: 15,
            },
            {
                name: "stone", // 섬록암
                id: 3,
                precent: 15,
            },
            { 
                name: "cobblestone",
                precent: 15,
            }, // 60
            {
                name: "coal_ore",
                precent: 7,
            },
            {
                name: "iron_ore",
                precent: 7,
            },
            {
                name: "gold_ore",
                precent: 7,
            },
            {
                name: "lapis_ore",
                precent: 7,
            },
            {
                name: "redstone_ore",
                precent: 7,
            },
            {
                name: "diamond_ore",
                precent: 5,
            },
        ]
    },
    {
        name: `${ChatColor.Bold}${ChatColor.DarkRed}HELL`,
        requiredBlock: 800,
        blocks: [
            {
                name: "Netherrack",
                precent: 17,
            },
            {
                name: "warped_nylium",
                precent: 9,
            },
            {
                name: "crimson_nylium",
                precent: 9,
            },
            {
                name: "bone_block"
                precent:5
            },
            {
                name: "quartz_ore",
                precent: 8,
            },
            {
                name: "nether_gold_ore",
                precent: 18,
            },
            {
                name: "glowstone",
                precent: 10,
            },
            {
                name: "warped_stem",
                precent: 10,
            },
            {
                name: "warped_wart_block",
                precent: 2
            },
            {
                name: "crimson_stem",
                precent: 10
            },
            {
                name: "crimson_hyphae",
                precent: 2
            }
        ]
    }
]
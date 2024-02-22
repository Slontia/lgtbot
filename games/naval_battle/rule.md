## 大海战

- **游戏人数：** 2
- **原作：** balk

### 游戏简介
- 玩家需要在地图上布置飞机，并击落对手的飞机

### 游戏流程
- 每个玩家需要在自己的地图中放置 3 架飞机，飞机形状固定为“士”字型（占10格），其中“士”的顶端称为“要害”（机头）
- **侦察点：** 游戏开始前，地图上会随机生成少量侦察点，放置侦察点的机身会**被显示**（机头不能放置于侦察点）
- 双方玩家同时行动，每回合可以向对方地图发射最多 3 枚导弹。当导弹命中敌机时，回合结束（剩余导弹不可发射）
- 若命中敌机“要害”，则额外奖励 1 回合（即可再发射最多 3 枚导弹）
- 先命中敌方全部“要害”的玩家获得胜利！如果双方玩家同时胜利，则**最后一回合行动次数少**的玩家获胜
- 默认规则不允许飞机重叠；命中“要害”时会告知
- **注意：** 当机头被命中后，该飞机其他位置受到攻击时依然提示命中

### 特殊规则
- 允许飞机重叠：若规则允许飞机重叠时，飞机的机身可任意数量重叠，机头不可与机身重叠
- 仅首“要害”公开：在该规则下，每个玩家命中过1次机头以后，之后再次命中其他机头时，仅告知命中，不提示命中要害，且不具有额外一回合
- 无“要害”提示：在该规则下，命中机头时，仅告知命中，不再提示命中要害，且不具有额外一回合。

### 地图图例
<table style="text-align:center; margin:auto; height:125px">
    <tr>
        <td style="background-color:#ECECEC; width:35px; border:1px solid black">　</td><td style="border:1px solid black">未被打击区域</td>
        <td style="background-color:#E0FFE0; width:35px; border:1px solid black"><span style="font-size:x-large;">+</span></td><td style="border:1px solid black">未被打击机身</td>
        <td style="background-color:#E0FFE0; width:35px; border:1px solid black"><span style="font-size:x-large; color:#FF0000;">★</span></td><td style="border:1px solid black">未被打击飞机头</td>
    </tr>
    <tr>
        <td style="background-color:#B0E0FF; width:35px; border:1px solid black">　</td><td style="border:1px solid black">已被打击空地</td>
        <td style="background-color:#FFA0A0; width:35px; border:1px solid black"><span style="font-size:x-large;">+</span></td><td style="border:1px solid black">已被打击机身</td>
        <td style="background-color:#000000; width:35px; border:1px solid black"><span style="font-size:x-large; color:#FF0000;">★</span></td><td style="border:1px solid black">已被打击飞机头</td>
    </tr>
    <tr>
        <td style="background-color:#A0FFA0; width:35px; border:1px solid black"><span style="font-size:x-large;">-</span></td><td style="border:1px solid black">侦察点：空地</td>
        <td style="background-color:#FF6868; width:35px; border:1px solid black"><span style="font-size:x-large;">+</span></td><td style="border:1px solid black">侦察点：机身</td>
    </tr>
</table>

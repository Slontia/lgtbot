## 扑克矩阵

- **游戏人数：** 任意

### 规则说明

每名玩家的盘面为 5 * 5 的矩阵，玩家需要按顺序将展示的扑克填入矩阵中的任意位置，直到填满整个盘面。

### 分数计算

矩阵填满之后，将会形成 12 组牌组（横向 5 组，纵向 5 组，对角线 2 组）。游戏会为每组牌单独计算出一个分数，所有牌组的分数累计，即是玩家取得的总分。

每组牌的分数计算公式为「卡牌点数总和 * 牌型点数 * 倍数 + 奖励分」。

卡牌的数字对应的点数为：

<table align="center" border="1px solid #ccc" cellpadding="5" cellspacing="1">
  <tbody>
    <tr>
      <td align=\"center\">2 ~ 6</td>
      <td align=\"center\">7 ~ 10</td>
      <td align=\"center\">J ~ K</td>
      <td align=\"center\">A</td>
    </tr>
    <tr>
      <td align=\"center\">1 点</td>
      <td align=\"center\">2 点</td>
      <td align=\"center\">3 点</td>
      <td align=\"center\">5 点</td>
    </tr>
  </tbody>
</table>

各个牌型对应的点数为：

<table align="center" border="1px solid #ccc" cellpadding="5" cellspacing="1">
  <tbody>
    <tr>
      <td align="center">高牌</td>
      <td align="center">一对</td>
      <td align="center">两对</td>
      <td align="center">三条</td>
      <td align="center">顺子</td>
      <td align="center">同花</td>
      <td align="center">满堂红</td>
      <td align="center">四条</td>
      <td align="center">同花顺</td>
    </tr>
    <tr>
      <td align="center">0 点</td>
      <td align="center">1 点</td>
      <td align="center">3 点</td>
      <td align="center">6 点</td>
      <td align="center">12 点</td>
      <td align="center">5 点</td>
      <td align="center">10 点</td>
      <td align="center">16 点</td>
      <td align="center">30 点</td>
    </tr>
  </tbody>
</table>

横纵的 10 组牌对应的倍数为 1，对角线的 2 组牌对应的倍数为 2。

奖励分包括两方面：在奖励格上直接取得的分数，和牌型种类达到一定数量的奖励分数。牌型种类数量（排除高牌，大同花顺被认为是同花顺）和得分的对应关系为：

<table align="center" border="1px solid #ccc" cellpadding="5" cellspacing="1">
  <tbody>
    <tr>
      <td align="center">5 种</td>
      <td align="center">6 种</td>
      <td align="center">7 种</td>
      <td align="center">8 种</td>
    </tr>
    <tr>
      <td align="center">100 分</td>
      <td align="center">200 分</td>
      <td align="center">400 分</td>
      <td align="center">800 分</td>
    </tr>
  </tbody>
</table>

### 奖励格

在 25 个格子中，有 4 种特殊格子。当玩家放置卡牌在特殊格子上时，会触发特殊效果。

- 黑奖励格：放置「黑桃」或「梅花」卡牌时，获得「20 × 卡牌点数」分
- 红奖励格：放置「红桃」或「方板」卡牌时，获得「20 × 卡牌点数」分
- 加倍格：本回合完成的牌型，得分倍率加 1
- 过牌格：跳过下一张卡牌的放置


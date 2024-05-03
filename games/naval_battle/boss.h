
#include <iomanip>

class Boss
{
public:

    // BOSS挑战奖励积分数
    int points = 0;

    // BOSS编号
    int BossType{};
    // 是否开启可重叠规则
    bool overlap{};
    // 是否为BOSS对战（BOSS被动技能判定）
    bool is_boss = false;

    // BOSS2 核弹研发中心
    bool RD_is_hit = false;
    const int RDcenter_position[8][2] = {{-2,0}, {-1,0}, {1,0}, {2,0}, {0,-2}, {0,-1}, {0,1}, {0,2}};

    // BOSS简介
    string BossDesc() const
    {
        if (BossType == 1) return "<div><b>无限火力BOSS战</b></div>拥有多种特殊范围技能，擅长对全图进行火力打击";
        if (BossType == 2) return "<div><b>核平铀好BOSS战</b></div>擅长干扰对手的导弹来拖延回合，研发核弹进行毁灭性打击";
        return "<div><b>BOSS类型错误</b></div>error BossType for BOSS " + to_string(BossType);
    }

    // BOSS技能介绍
    string BossIntro() const
    {
        const string empty = "<td style=\"background-color:#ECECEC;width:25px;\">　</td>";
        const string body = "<td style=\"background-color:#E0FFE0;width:25px;\">+</td>";
        const string head = "<td style=\"background-color:#000000;width:25px;\"><font color=\"FF0000\">★</font></td>";
        const string S_head = "<td style=\"background-color:#5A5A5A;width:25px;\"><font color=\"FFE119\">⚛</font></td>";

        string BOSS_SkillIntro = "<table>";
        if (BossType == 1) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">无限火力BOSS战</th></tr>";
            BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个技能，且都会进行普通打击：随机向地图上发射 3-6 枚导弹。当达到 18 回合时，BOSS将增强普攻至最多 8 枚导弹</td></tr>";
            BOSS_SkillIntro += "<tr><td>技能1：15% 概率发动 [空军指挥]——随机移动所有未被击落的飞机至其他位置，并尽可能避开已侦察区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>技能3：15% 概率发动 [连环轰炸]——随机打击地图上某个坐标的整个十字区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>技能2：15% 概率发动 [雷达扫描]——随机扫描地图上 5*5 的区域，其中的所有飞机（飞机头+机身）会被直接击落</td></tr>";
            BOSS_SkillIntro += "<tr><td>技能4：25% 概率发动 [火力打击]——BOSS会发射一枚高爆导弹，炸毁地图上 3*3 的区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>特殊：当BOSS的有 3 架飞机被击落时，所有技能的触发概率提升 5%！</td></tr>";
        }
        if (BossType == 2) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">核平铀好BOSS战</th></tr>";

            BOSS_SkillIntro += "<tr><td>【特殊飞机】核弹研发中心：呈十字型，要害在其中心，形状见下图。被打击要害时使核弹发射基础概率减少 10%（此要害不计算在总要害数中）</td></tr>";
            BOSS_SkillIntro += "<tr><td><table style=\"text-align:center;margin:auto\">";
            BOSS_SkillIntro += "<tr>" + empty + empty + body   + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + body   + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + body  + body  + S_head + body  + body  + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + body   + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + body   + empty + empty + "</tr>";
            BOSS_SkillIntro += "</table></td></tr>";

            BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个主动技能，且都会进行普通打击：随机向地图上发射 3-6 枚导弹。当达到 18 回合时，BOSS将增强普攻至最多 8 枚导弹</td></tr>";
            BOSS_SkillIntro += "<tr><td>主动技能1：[核弹]——摧毁整个地图。基础概率为0，每回合概率提升 0.5%；BOSS每有一个机身被打击，概率提升 0.1%；每有一个要害被打击，概率提升 0.5%；可通过打击研发中心来延缓进展。</td></tr>";
            BOSS_SkillIntro += "<tr><td>主动技能2：5% 概率发动 [空军支援]——将一架已被击落的飞机更换为新飞机，并转移位置。如果BOSS没有被击落的飞机，会额外起飞一架（最多不超过 6 架）</td></tr>";
            BOSS_SkillIntro += "<tr><td>主动技能3：15% 概率发动 [石墨炸弹]——在发动技能的回合，玩家仅有 1 枚导弹。</td></tr>";
            BOSS_SkillIntro += "<tr><td>被动技能：玩家每次攻击时都有 8% 概率触发 [导弹拦截]，拦截玩家的导弹并打击到玩家自己地图的相同位置，小心了！</td></tr>";
        }
        if (BossType == 3) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">？？？BOSS战</th></tr>";

            BOSS_SkillIntro += "<tr><td>【特殊飞机】？？？：？？？</td></tr>";
            BOSS_SkillIntro += "<tr><td><table style=\"text-align:center;margin:auto\">";
            BOSS_SkillIntro += "<tr>" + empty + empty + empty + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + empty + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + S_head + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + empty + empty + empty + "</tr>";
            BOSS_SkillIntro += "<tr>" + empty + empty + empty + empty + empty + "</tr>";
            BOSS_SkillIntro += "</table></td></tr>";

            BOSS_SkillIntro += "<tr><td></td></tr>";
        }
        BOSS_SkillIntro += "</table>";
        return BOSS_SkillIntro;
    }

    // BOSS放置阶段
    void BossPrepare(Board (&board)[2])
    {
        int X, Y, direction;
        string str;
        RD_is_hit = false;
        bool RD_success = false;
        int try_count = 0;

        while (board[1].alive < board[1].planeNum) {
            // BOSS2 放置核弹研发中心
            if (BossType == 2) {
                while (!RD_success) {
                    X = rand() % (board[1].sizeX - 4) + 3;
                    Y = rand() % (board[1].sizeY - 4) + 3;
                    if (board[1].map[X][Y][0] == 0) {
                        board[1].map[X][Y][1] = 3;
                        for (auto position : RDcenter_position) {
                            board[1].map[X + position[0]][Y + position[1]][1] = 1;
                            board[1].body[X + position[0]][Y + position[1]] += 1;
                        }
                        RD_success = true;
                    }
                }
            }
            // 基础飞机放置
            X = rand() % board[1].sizeX + 1;
            Y = rand() % board[1].sizeY + 1;
            str = string(1, 'A' + X - 1) + to_string(Y);
            direction = rand() % 4 + 1;
            if (board[1].AddPlane(str, direction, overlap) == "OK") try_count = 0;

            if (try_count++ > 5000) {
                board[1].RemoveAllPlanes();
                try_count = 0;
                RD_success = false;
            }
        }
    }

    // BOSS 普攻
    static string BossNormalAttack(Board (&board)[2], int round_, int (&attack_count)[2])
    {
        int X, Y;
        string str;
        int try_count = 0, count = 0;
        int num;
        if (round_ < 18) {
            num = rand() % 4 + 3;
        } else {
            num = rand() % 4 + 5;
        }
        for (int i = 0; i < num; i++) {
            if (try_count > 1000) break;
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            str = string(1, 'A' + X - 1) + to_string(Y);
            string result = board[0].Attack(str);
            if (result != "0" && result != "1" && result != "2" ) {
                i--; try_count++;
                continue;
            } else {
                count++;
            }
        }
        attack_count[1] = count;
        return string(round_ == 18 ? "【BOSS】普通打击已升级，每回合最多发射 8 枚导弹\n" : "") + "BOSS进行普通打击：共发射了 " + to_string(count) + " 枚导弹";
    }


    // BOSS 技能攻击
    string BossSkillAttack(Board (&board)[2], int round, int (&timeout)[2], int (&repeated)[2]) const
    {
        string skillinfo;
        if (BossType == 1) skillinfo = Boss1_SkillAttack(board, timeout);
        if (BossType == 2) skillinfo = Boss2_SkillAttack(board, round, repeated, timeout);

        if (skillinfo.empty()) return "";

        return "\n\n" + skillinfo;
    }


    // BOSS1 技能攻击
    string Boss1_SkillAttack(Board (&board)[2], int (&timeout)[2]) const
    {
        const int skill_probability[4] = {15, 30, 45, 70};
        int add_p = 0;
        int X, Y;
        string str;

        int skill = rand() % 100 + 1;
        if (board[1].alive <= board[1].planeNum - 3) add_p = 5;

        if (skill > 100 - skill_probability[0] - add_p * 1)
        {
            // 15%概率触发空军指挥，移动剩余飞机
            int alive_count = 0;
            for(int j = 1; j <= board[1].sizeY; j++) {
                for(int i = 1; i <= board[1].sizeX; i++) {
                    if (board[1].map[i][j][1] == 2 && board[1].map[i][j][0] == 0) {
                        str = string(1, 'A' + i - 1) + to_string(j);
                        board[1].RemovePlane(str);
                        alive_count++;
                    }
                }
            }
            int try_count = 0;
            int direction;
            while (board[1].alive < alive_count) {
                X = rand() % board[1].sizeX + 1;
                Y = rand() % board[1].sizeY + 1;
                str = string(1, 'A' + X - 1) + to_string(Y);
                direction = rand() % 4 + 1;
                int found_count = 0;
                for (int i = 0; i < 9; i++) {
                    if (board[1].map[X + board[1].position[direction][i][0]][Y + board[1].position[direction][i][1]][0] > 0) {
                        found_count++;
                    }
                }
                bool hide = false;
                for (int i = 0; i <= 9; i++) {
                    if ((found_count <= i && try_count >= i * 300) || (found_count <= 1 && rand() % 80 == 0)) {
                        hide = true; break;
                    }
                }
                if (board[1].map[X][Y][0] == 0 && hide) {
                    board[1].AddPlane(str, direction, overlap) == "OK" ? try_count = 0 : try_count++;
                }
                if (try_count > 3000) {
                    timeout[1] = 1;
                    board[1].alive = 0;
                    return "[系统错误] BOSS技能阶段——空军指挥：移动 " + to_string(alive_count) + " 架飞机时发生错误，随机放置飞机次数到达上限，已强制中断游戏进程！";
                }
            }
            return "【WARNING】BOSS发动技能 [空军指挥]！已指挥 " + to_string(alive_count) + " 架飞机飞行至地图的其他位置";
        }
        else if (skill > 100 - skill_probability[1] - add_p * 2)
        {
            // 15%概率触发连环轰炸，打击十字区域
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            for (int i = 0; i < board[0].sizeX; i++) {
                str = string(1, 'A' + X - 1) + to_string(i + 1);
                board[0].Attack(str);
                str = string(1, 'A' + i) + to_string(Y);
                board[0].Attack(str);
            }
            return "【WARNING】BOSS发动技能 [连环轰炸]！对地图行列进行打击，打击了地图上 " + string(1, 'A' + X - 1) + to_string(Y) + " 所在的整个十字区域";
        }
        else if (skill > 100 - skill_probability[2] - add_p * 3)
        {
            // 15%概率触发雷达扫描，扫描5*5，击落所有飞机
            int count, exposed_space_count, exposed_plane_count;
            bool remain_head;
            for (int attempt = 1; attempt <= 30; attempt++) {
                count = exposed_space_count = exposed_plane_count = 0;
                remain_head = true;
                X = rand() % (board[0].sizeX - 4) + 3;
                Y = rand() % (board[0].sizeY - 4) + 3;
                for (int i = -2; i <= 2; i++) {
                    for (int j = -2; j <= 2; j++) {
                        if (board[0].map[X + i][Y + j][0] > 0) {
                            if (board[0].map[X + i][Y + j][1] == 0) exposed_space_count++;
                            if (board[0].map[X + i][Y + j][1] == 1) exposed_plane_count++;
                            if (board[0].map[X + i][Y + j][1] == 2) remain_head = false;
                        }
                    }
                }
                if ((remain_head && exposed_plane_count >= 3) || exposed_space_count <= 10 || attempt == 30) {
                    for (int i = -2; i <= 2; i++) {
                        for (int j = -2; j <= 2; j++) {
                            str = string(1, 'A' + X + i - 1) + to_string(Y + j);
                            if (board[0].map[X + i][Y + j][1] > 0 && board[0].map[X + i][Y + j][0] == 0) {
                                board[0].Attack(str);
                                count++;
                            }
                        }
                    }
                    break;
                }
            }
            if (count > 0) {
                return "【WARNING】BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + string(1, 'A' + X - 1) + to_string(Y) + " 为中心的 5*5 的区域，并发射 " + to_string(count) + " 枚导弹打击了检测到的所有飞机";
            } else {
                return "【WARNING】BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + string(1, 'A' + X - 1) + to_string(Y) + " 为中心的 5*5 的区域，什么都没有发现";
            }
        }
        else if (skill > 100 - skill_probability[3] - add_p * 4)
        {
            // 25%概率触高爆导弹，打击3*3区域
            for (int attempt = 1; attempt <= 30; attempt++) {
                int exposed_count = 0;
                X = rand() % (board[0].sizeX - 2) + 2;
                Y = rand() % (board[0].sizeY - 2) + 2;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (board[0].map[X + i][Y + j][0] > 0) {
                            exposed_count++;
                        }
                    }
                }
                if (exposed_count <= 4 || attempt == 30) {
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            str = string(1, 'A' + X + i - 1) + to_string(Y + j);
                            board[0].Attack(str);
                        }
                    }
                    break;
                }
            }
            return "【WARNING】BOSS发动技能 [火力打击]！发射了一枚高爆导弹，炸毁了位于 " + string(1, 'A' + X - 1) + to_string(Y) + " 的 3*3 的区域";
        }
        return "";
    }


    // BOSS2 技能攻击
    string Boss2_SkillAttack(Board (&board)[2], int round, int (&repeated)[2], int (&timeout)[2]) const
    {
        const int skill_probability[2] = {5, 20};
        string N_warning;
        int X, Y;
        string str;
        
        int skill = rand() % 100 + 1;

        // [核弹]
        int percent = round * 5;
        if (RD_is_hit) percent -= 100;
        for (int i = 1; i <= board[0].sizeX; i++) {
            for (int j = 1; j <= board[0].sizeY; j++) {
                if (board[1].map[i][j][0] == 1) {
                    if (board[1].map[i][j][1] == 2) {
                        percent += 5;
                    } else if (board[1].map[i][j][1] == 1) {
                        percent += 1;
                    }
                }
            }
        }
        double percent_d = percent / 10.0;
        ostringstream oss;
        oss << fixed << setprecision(1) << percent_d;
        string percent_s = oss.str();
        if (rand() % 1000 < percent) {
            for (int i = 1; i <= board[0].sizeX; i++) {
                for (int j = 1; j <= board[0].sizeY; j++) {
                    str = string(1, 'A' + i - 1) + to_string(j);
                    board[0].Attack(str);
                }
            }
            return "【NUCLEAR SIREN】[核弹] 研发成功并被发射！地图全部区域被摧毁（概率 " + percent_s + "% 已触发）";
        } else if (percent >= 60 && !RD_is_hit) {
            if (skill > 100 - skill_probability[1]) N_warning = "\n";
            N_warning += "【NUCLEAR SIREN】核弹研发成功概率已达到 " + percent_s + "%，尽快击毁研发中心以拖慢进展";
        }

        if (skill > 100 - skill_probability[0])
        {
            // 5%概率发动空军支援，复活或支援新飞机
            bool found = false;
            for(int j = 1; j <= board[1].sizeY; j++) {
                for(int i = 1; i <= board[1].sizeX; i++) {
                    if (board[1].map[i][j][1] == 2 && board[1].map[i][j][0] == 1 && !found) {
                        str = string(1, 'A' + i - 1) + to_string(j);
                        board[1].RemovePlane(str);
                        found = true;
                    }
                }
            }
            if (!found && board[1].planeNum == 6) {
                return "【WARNING】BOSS发动技能 [空军支援]！但飞机数已达上限，支援发动失败" + N_warning;
            }
            int try_count = 0;
            int direction;
            bool success = false;
            while (!success) {
                X = rand() % board[1].sizeX + 1;
                Y = rand() % board[1].sizeY + 1;
                str = string(1, 'A' + X - 1) + to_string(Y);
                direction = rand() % 4 + 1;
                int found_count = 0;
                for (int i = 0; i < 9; i++) {
                    if (board[1].map[X + board[1].position[direction][i][0]][Y + board[1].position[direction][i][1]][0] > 0) {
                        found_count++;
                    }
                }
                bool hide = false;
                for (int i = 0; i <= 9; i++) {
                    if (found_count <= i && try_count >= i * 300) {
                        hide = true;
                    }
                }
                if (board[1].map[X][Y][0] == 0 && hide) {
                    if (board[1].AddPlane(str, direction, overlap) == "OK") success = true;
                }
                if (try_count++ > 3000) {
                    timeout[1] = 1;
                    board[1].alive = 0;
                    return "[系统错误] BOSS技能阶段——空军支援：随机放置飞机次数到达上限，已强制中断游戏进程！";
                }
            }
            if (found) {
                board[1].alive++;
                return "【WARNING】BOSS发动技能 [空军支援]！已成功将一架已被击落的飞机更换为新飞机，并转移位置" + N_warning;
            } else {
                board[1].planeNum++;
                return "【WARNING】BOSS发动技能 [空军支援]！一架新起飞的飞机抵达战场！当前总飞机数为 6" + N_warning;
            }
        }
        else if (skill > 100 - skill_probability[1])
        {
            // 10%概率发动石墨炸弹，限制导弹数量
            repeated[0] = 1;
            return "【WARNING】BOSS发动技能 [石墨炸弹]！在本回合，您仅有 1 枚导弹" + N_warning;
        }
        return N_warning;
    }


    // BOSS 被动技能
    pair<string, string> BOSS_PassiveSkills(Board (&board)[2], string str) const
    {
        if (is_boss) {
            // BOSS2 [导弹拦截]
            if (BossType == 2) {
                if (rand() % 100 < 8) {
                    string ret = board[0].Attack(str);
                    if (ret == "0" || ret == "1" || ret == "2") {
                        return make_pair(ret, "【WARNING】BOSS触发技能 [导弹拦截]，当前导弹被拦截并打击到了玩家的地图上");
                    }
                }
            }
        }
        return make_pair("Failed", "Empty");
    }

    // BOSS 特殊飞机被打击
    string BOSS_SpecialPlanes(Board (&board)[2], string str)
    {
        string info = "导弹击中BOSS特殊要害！\n本回合结束。";
        // BOSS2 研发中心被击中&自动标记
        if (BossType == 2) {
            RD_is_hit = true;
            Board::CheckCoordinate(str);
            int X = str[0] - 'A' + 1, Y = str[1] - '0'; 
            if (str.length() == 3) {
                Y = (str[1] - '0') * 10 + str[2] - '0';
            }
            for (auto position : RDcenter_position) {
                board[1].mark[X + position[0]][Y + position[1]] = 3;
            }
            return "核弹研发中心被击中！研发成功概率已大幅下降。\n" + info;
        }
        return info;
    }

};

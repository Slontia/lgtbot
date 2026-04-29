
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


    // BOSS0 临时类型&万能核心
    int tempBossType{};
    const vector<pair<int, int>> UniversalCore_position = {{-1,0}, {1,0}, {0,-1}, {0,1}};
    const vector<pair<int, int>> UniversalCoreRandom_position = {{-1,-1}, {-1,1}, {1,-1}, {1,1}};

    // BOSS2 核弹研发中心
    bool nuclear_hitted = false;
    bool RD_is_hit = false;
    const vector<pair<int, int>> RDcenter_position = {{-2,0}, {-1,0}, {1,0}, {2,0}, {0,-2}, {0,-1}, {0,1}, {0,2}};

    // BOSS3 电磁干扰
    bool EMI = false;


    // BOSS简介
    string BossDesc() const
    {
        if (BossType == 0) return "<div><b>？？？BOSS战</b></div>每回合随机改变形态，可以从所有BOSS中变换技能";
        if (BossType == 1) return "<div><b>无限火力BOSS战</b></div>拥有多种特殊范围技能，擅长对全图进行火力打击";
        if (BossType == 2) return "<div><b>核平铀好BOSS战</b></div>擅长干扰对手的导弹来拖延回合，研发核弹进行毁灭性打击";
        if (BossType == 3) return "<div><b>科技时代BOSS战</b></div>装备多种高科技武器以进行攻击和干扰对方的导弹";
        return "<div><b>BOSS类型错误</b></div>error BossType for BOSS " + to_string(BossType);
    }

    // BOSS技能介绍
    string BossIntro() const
    {
        string BOSS_SkillIntro = "<table>";
        if (BossType == 0) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">？？？BOSS战</th></tr>";
            BOSS_SkillIntro += "<tr><td>【特殊飞机】万能核心：形状不固定，要害在其中心，随机在某个编号对应的位置上安置4个机身。被打击要害时视为所有BOSS形态的特殊要害被击中（此要害不计算在总要害数中）</td></tr>";
            BOSS_SkillIntro += "<tr><td>";
            BOSS_SkillIntro += Board::GetPlaneTable({"45054", "67176", "01310", "67176", "45054"}, 1);
            BOSS_SkillIntro += "</td></tr>";
            BOSS_SkillIntro += "<tr><td>【变换技能】每回合会从所有BOSS中随机一个并改变形态，根据变换对应的BOSS发动普通打击和主动技能。被动技能仅在切换至对应的BOSS时才有可能触发。</td></tr>";
        }
        if (BossType == 1) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">无限火力BOSS战</th></tr>";
            BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个技能，且都会进行普通打击：随机向地图上发射 1-4 枚导弹。当达到 20 回合时，BOSS将增强普攻至 4-6 枚导弹</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能1】15% 概率发动 [空军指挥]——随机移动所有未被击落的飞机至其他位置，并尽可能避开已侦察区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能3】15% 概率发动 [连环轰炸]——随机打击地图上某个坐标的整个十字区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能2】15% 概率发动 [雷达扫描]——随机扫描地图上 5*5 的区域，其中的所有飞机（飞机头+机身）会被直接击落</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能4】25% 概率发动 [火力打击]——BOSS会发射一枚高爆导弹，炸毁地图上 3*3 的区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>【被动技能】当BOSS有 3 架飞机被击落时，所有技能的触发概率提升 5%！</td></tr>";
        }
        if (BossType == 2) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">核平铀好BOSS战</th></tr>";

            BOSS_SkillIntro += "<tr><td>【特殊飞机】核弹研发中心：呈十字形，要害在其中心，形状见下图。被打击要害时使核弹发射基础概率减少 10%（此要害不计算在总要害数中）</td></tr>";
            BOSS_SkillIntro += "<tr><td>";
            BOSS_SkillIntro += Board::GetPlaneTable({"00100", "00100", "11311", "00100", "00100"}, 1);
            BOSS_SkillIntro += "</td></tr>";

            BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个主动技能，且都会进行普通打击：随机向地图上发射 3-6 枚导弹。当达到 18 回合时，BOSS将增强普攻至 6-8 枚导弹</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能1】[核弹]——摧毁整个地图，但如果概率低于 6% 时引爆威力会下降。基础概率为0，每回合概率提升 0.5%；BOSS每有一个机身被打击，概率提升 0.1%；每有一个要害被打击，概率提升 0.5%；可通过打击研发中心来延缓进展</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能2】5% 概率发动 [空军支援]——将一架已被击落的飞机更换为新飞机，并转移位置。如果BOSS没有被击落的飞机，会额外起飞一架（最多不超过 6 架）</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能3】15% 概率发动 [石墨炸弹]——发动技能的回合，玩家仅有 1 枚导弹</td></tr>";
            BOSS_SkillIntro += "<tr><td>【被动技能】玩家每次攻击时都有 8% 概率触发 [导弹拦截]，拦截玩家的导弹并打击到玩家自己地图的相同位置，小心了！</td></tr>";
        }
        if (BossType == 3) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">科技时代BOSS战</th></tr>";
            BOSS_SkillIntro += "<tr><td>BOSS每回合最多发动一个技能，且都会进行普通打击：随机向地图上发射 3-5 枚导弹。</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能1】10% 概率发动 [高能激光]——从地图边缘随机一格射入激光进行斜线打击，在到达地图边缘后会反射两次</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能2】10% 概率发动 [热跟踪弹]——随机跟踪一个已知的机身位置，并打击附近的 3*3 区域</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能3】15% 概率发动 [自爆无人机]——随机对 3 个横竖各5格共9格的十字形区域进行打击</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能4】15% 概率发动 [分导弹头]——每发导弹落点 5*5 范围内再随机打击 3 个格子</td></tr>";
            BOSS_SkillIntro += "<tr><td>【主动技能5】15% 概率发动 [电磁干扰]——本回合玩家的所有导弹实际落点会在选择的坐标周围 5*5 区域内发生偏移</td></tr>";
            BOSS_SkillIntro += "<tr><td>【被动技能】当BOSS有 3 架飞机被击落时，技能5[电磁干扰]将不能发动，但使其他所有技能的触发概率提升 5%，同时技能3[自爆无人机]数量变为 2 个</td></tr>";
        }
        if (BossType == 4) {
            BOSS_SkillIntro += "<tr><th style=\"text-align:center;\">【？】BOSS战</th></tr>";

            BOSS_SkillIntro += "<tr><td>【特殊飞机】？？？：（此要害不计算在总要害数中）</td></tr>";
            BOSS_SkillIntro += "<tr><td>";
            BOSS_SkillIntro += Board::GetPlaneTable({"01010", "12021", "00300", "12021", "01010"}, 1);
            BOSS_SkillIntro += "</td></tr>";

            BOSS_SkillIntro += "<tr><td>【】</td></tr>";
        }
        BOSS_SkillIntro += "</table>";
        return BOSS_SkillIntro;
    }

    // BOSS放置阶段（放置阶段调用）
    void BossPrepare(Board (&board)[2], int (&timeout)[2])
    {
        // 特殊飞机放置
        BossSpecialPlanesPrepare(board);
        // 基础飞机放置
        int X, Y, direction;
        RD_is_hit = false;
        int try_count = 0;
        int clear_count = 0;
        while (board[1].alive < board[1].planeNum * board[1].crucial_num) {
            X = rand() % board[1].sizeX + 1;
            Y = rand() % board[1].sizeY + 1;
            direction = rand() % 4 + 1;
            if (board[1].AddPlane(X, Y, direction, overlap) == "OK") try_count = 0;
            if (try_count++ > 5000) {
                if (clear_count++ > 10) {
                    timeout[1] = 1;
                    return;
                }
                board[1].RemoveAllPlanes();
                try_count = 0;
                BossSpecialPlanesPrepare(board);
            }
        }
    }

    // BOSS特殊飞机放置
    void BossSpecialPlanesPrepare(Board (&board)[2])
    {
        bool SpecialPlane_success = false;
        while (!SpecialPlane_success) {
            int X = rand() % (board[1].sizeX - 4) + 3;
            int Y = rand() % (board[1].sizeY - 4) + 3;
            if (board[1].map[X][Y][0] > 0) continue;
            if (BossType == 0) {
                // BOSS0 放置万能核心
                board[1].map[X][Y][1] = 3;
                for (auto position : UniversalCore_position) {
                    board[1].map[X + position.first][Y + position.second][1] = board[1].body[X + position.first][Y + position.second] = 1;
                }
                const int a = rand() % 2 + 1, b = rand() % 2 + 1;
                for (auto position : UniversalCoreRandom_position) {
                    board[1].map[X + a * position.first][Y + b * position.second][1] = board[1].body[X + a * position.first][Y + b * position.second] = 1;
                }
                SpecialPlane_success = true;
            } else if (BossType == 2) {
                // BOSS2 放置核弹研发中心
                board[1].map[X][Y][1] = 3;
                for (auto position : RDcenter_position) {
                    board[1].map[X + position.first][Y + position.second][1] = board[1].body[X + position.first][Y + position.second] = 1;
                }
                SpecialPlane_success = true;
            } else {
                // 无特殊要害
                return;
            }
        }
    }


    // BOSS 普攻+技能攻击（进攻阶段调用）
    string BossAttack(Board (&board)[2], int round, int (&attack_count)[2], int (&timeout)[2], int (&repeated)[2])
    {
        if (BossType == 0) tempBossType = rand() % 3 + 1;
        string normalInfo = (BossType == 0 ? "[BOSS " + to_string(tempBossType) + " 形态]\n" : "");
        if (BossType == 1 || tempBossType == 1) normalInfo += BossNormalAttack(board, round, attack_count, 1, 4, 20, 2);
        if (BossType == 2 || tempBossType == 2) normalInfo += BossNormalAttack(board, round, attack_count, 3, 6, 18, 2);
        if (BossType == 3 || tempBossType == 3) normalInfo += BossNormalAttack(board, round, attack_count, 3, 5, 99, 0);

        string skillInfo;
        if (BossType == 1 || tempBossType == 1) skillInfo += Boss1_SkillAttack(board, timeout);
        if (BossType == 2 || tempBossType == 2) skillInfo += Boss2_SkillAttack(board, round, repeated, timeout);
        if (BossType == 3 || tempBossType == 3) skillInfo += Boss3_SkillAttack(board);
        skillInfo = (skillInfo.empty() ? "" : "\n\n") + skillInfo;

        return normalInfo + skillInfo + "\n";
    }


    // BOSS 普攻
    static string BossNormalAttack(Board (&board)[2], const int round_, int (&attack_count)[2], const int a, const int b, const int enhance_round, const int enhance_num)
    {
        int num = (round_ < enhance_round ? rand() % (b - a + 1) + a : rand() % (b - a + 1) + a + enhance_num);
        int X, Y, try_count = 0, count = 0;
        for (int i = 0; i < num; i++) {
            if (try_count > 1000) break;
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            string result = board[0].Attack(X, Y);
            if (result != "0" && result != "1" && result != "2" ) {
                i--; try_count++;
                continue;
            } else {
                count++;
            }
        }
        attack_count[1] = count;
        return string(round_ == enhance_round ? "【BOSS】普通打击已升级，每回合最多发射 " + to_string(b + enhance_num) + " 枚导弹\n" : "") + "BOSS进行普通打击：共发射了 " + to_string(count) + " 枚导弹";
    }


    // BOSS1 技能攻击
    string Boss1_SkillAttack(Board (&board)[2], int (&timeout)[2]) const
    {
        const int skill_probability[4] = {15, 30, 45, 70};
        int add_p = 0;
        int X, Y;

        int skill = rand() % 100 + 1;
        if (board[1].alive <= (board[1].planeNum - 3) * board[1].crucial_num) add_p = 5;

        if (skill > 100 - skill_probability[0] - add_p * 1)
        {
            // 15%概率触发空军指挥，移动剩余飞机
            int alive_count = 0;
            for(int j = 1; j <= board[1].sizeY; j++) {
                for(int i = 1; i <= board[1].sizeX; i++) {
                    if (board[1].map[i][j][1] == 2 && board[1].map[i][j][0] == 0 && board[1].body[i][j] > 0) {
                        board[1].RemovePlane(i, j);
                        alive_count++;
                    }
                }
            }
            int try_count = 0;
            int direction;
            while (board[1].alive < alive_count * board[1].crucial_num) {
                X = rand() % board[1].sizeX + 1;
                Y = rand() % board[1].sizeY + 1;
                direction = rand() % 4 + 1;
                int found_count = 0;
                for (auto position : board[1].positions[direction]) {
                    if (board[1].map[X + position.first][Y + position.second][0] > 0) {
                        found_count++;
                    }
                }
                bool hide = false;
                for (int i = 0; i <= board[1].positions[1].size(); i++) {
                    if ((found_count <= i && try_count >= i * 300) || (found_count <= 1 && rand() % 50 == 0)) {
                        hide = true; break;
                    }
                }
                if (board[1].map[X][Y][0] == 0 && hide) {
                    board[1].AddPlane(X, Y, direction, overlap) == "OK" ? try_count = 0 : try_count++;
                }
                if (try_count > 3000) {
                    timeout[1] = 1;
                    return "[漏洞监测] BOSS技能阶段——空军指挥：移动 " + to_string(alive_count) + " 架飞机时发生错误，随机放置飞机次数到达上限，已强制中断游戏进程！";
                }
            }
            return "【WARNING】BOSS发动技能 [空军指挥]！已指挥 " + to_string(alive_count) + " 架飞机飞行至地图的其他位置";
        }
        else if (skill > 100 - skill_probability[1] - add_p * 2)
        {
            // 15%概率触发连环轰炸，打击十字区域
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            for (int i = 1; i <= board[0].sizeX; i++) {
                board[0].Attack(X, i);
                board[0].Attack(i, Y);
            }
            return "【WARNING】BOSS发动技能 [连环轰炸]！对地图行列进行打击，打击了地图上 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 所在的整个十字区域";
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
                            if (board[0].map[X + i][Y + j][1] > 0 && board[0].map[X + i][Y + j][0] == 0) {
                                board[0].Attack(X + i, Y + j);
                                count++;
                            }
                        }
                    }
                    break;
                }
            }
            if (count > 0) {
                return "【WARNING】BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 为中心的 5*5 的区域，并发射 " + to_string(count) + " 枚导弹打击了检测到的所有飞机";
            } else {
                return "【WARNING】BOSS发动技能 [雷达扫描]！使用雷达扫描了 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 为中心的 5*5 的区域，但什么都没有发现";
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
                            board[0].Attack(X + i, Y + j);
                        }
                    }
                    break;
                }
            }
            return "【WARNING】BOSS发动技能 [火力打击]！发射了一枚高爆导弹，炸毁了位于 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 的 3*3 的区域";
        }
        return "";
    }


    // BOSS2 技能攻击
    string Boss2_SkillAttack(Board (&board)[2], int round, int (&repeated)[2], int (&timeout)[2])
    {
        const int skill_probability[2] = {5, 20};
        string N_warning;
        int X, Y;
        
        int skill = rand() % 100 + 1;

        // [核弹]
        if (!nuclear_hitted) {
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
                nuclear_hitted = true;
                int power = rand() % 4 + 4;
                for (int i = 1; i <= board[0].sizeX; i++) {
                    for (int j = 1; j <= board[0].sizeY; j++) {
                        if (percent < 60 && rand() % 10 >= power) continue;
                        board[0].Attack(i, j);
                    }
                }
                if (percent >= 60) {
                    return "【NUCLEAR SIREN】[核弹] 研发成功并被发射！地图全部区域被摧毁（概率 " + percent_s + "% 已触发）";
                } else if (!RD_is_hit) {
                    return "【NUCLEAR SIREN】[核弹] 已被发射！（概率 " + percent_s + "% 已触发）但因研发过速，实际威力仅有预期的 " + to_string(power) + "0%";
                } else {
                    return "【NUCLEAR SIREN】[核弹] 已被发射！（概率 " + percent_s + "% 已触发）但因研发进展被阻挠，实际威力仅有预期的 " + to_string(power) + "0%";
                }
            } else if (percent >= 60 && !RD_is_hit) {
                if (skill > 100 - skill_probability[1]) N_warning = "\n";
                N_warning += "【NUCLEAR SIREN】核弹研发成功概率已达到 " + percent_s + "%，尽快击毁研发中心以拖慢进展";
            }
        }

        if (skill > 100 - skill_probability[0])
        {
            // 5%概率发动空军支援，复活或支援新飞机
            bool found = false;
            if (board[1].crucial_num == 1) {    // 仅单要害模式移除飞机
                for(int j = 1; j <= board[1].sizeY; j++) {
                    for(int i = 1; i <= board[1].sizeX; i++) {
                        if (board[1].map[i][j][1] == 2 && board[1].map[i][j][0] == 1 && !found) {
                            board[1].RemovePlane(i, j);
                            found = true;
                        }
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
                direction = rand() % 4 + 1;
                int found_count = 0;
                for (auto position : board[1].positions[direction]) {
                    if (board[1].map[X + position.first][Y + position.second][0] > 0) {
                        found_count++;
                    }
                }
                bool hide = false;
                for (int i = 0; i <= board[1].positions[1].size(); i++) {
                    if (found_count <= i && try_count >= i * 300) {
                        hide = true;
                    }
                }
                if (board[1].map[X][Y][0] == 0 && hide) {
                    if (board[1].AddPlane(X, Y, direction, overlap) == "OK") success = true;
                }
                if (try_count++ > 3000 && !success) {
                    return "【WARNING】BOSS发动技能 [空军支援]！随机放置飞机次数到达上限，技能发动失败！";
                }
            }
            if (found) {
                board[1].alive += board[1].crucial_num;
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


    // BOSS3 技能攻击
    string Boss3_SkillAttack(Board (&board)[2])
    {
        const int skill_probability[5] = {10, 20, 35, 50, 65};
        int add_p = 0;
        int X, Y;
        EMI = false;

        int skill = rand() % 100 + 1;
        if (board[1].alive <= board[1].planeNum * board[1].crucial_num - 3 * board[1].crucial_num) add_p = 5;

        if (skill > 100 - skill_probability[0] - add_p * 1)
        {
            // 10%概率发动高能激光（地图右侧2-13），斜线打击，反射两次
            const int laser_move[5][2] = {{}, {-1, 1}, {-1, -1}, {1, -1}, {1, 1}};   // 1左下 2左上 3右上 4右下
            int direction, reflex_count = 0;
            if (rand() % 2 == 0) {
                X = board[0].sizeX;
                direction = rand() % 2 + 1;
            } else {
                X = 1;
                direction = rand() % 2 + 3;
            }
            Y = rand() % 10 + 3;
            string start_str = string(1, 'A' + X - 1) + to_string(Y);
            while (reflex_count <= 2) {
                if ((X == 1 && direction == 2) || (X == board[0].sizeX && direction == 4) || (Y == 1 && direction == 3) || (Y == board[0].sizeY && direction == 1)) {
                    direction++;
                    reflex_count++;
                }
                if ((X == 1 && direction == 1) || (X == board[0].sizeX && direction == 3) || (Y == 1 && direction == 2) || (Y == board[0].sizeY && direction == 4)) {
                    direction--;
                    reflex_count++;
                }
                direction = direction == 0 ? 4 : (direction == 5 ? 1 : direction);
                board[0].Attack(X, Y);
                X += laser_move[direction][0];
                Y += laser_move[direction][1];
            }
            return "【WARNING】BOSS发动技能 [高能激光]！从 " + start_str + " 射入激光进行斜线打击，并在到达边缘后反射了两次";
        }
        else if (skill > 100 - skill_probability[1] - add_p * 2)
        {
            // 10%概率发动热跟踪弹，随机打击一个已知机身的附近3*3
            vector<vector<int>> found_body;
            for(int i = 1; i <= board[0].sizeX; i++) {
                for(int j = 1; j <= board[0].sizeY; j++) {
                    if (board[0].map[i][j][0] > 0 && board[0].map[i][j][1] == 1) {
                        found_body.push_back(vector<int>{i, j});
                    }
                }
            }
            if (found_body.empty()) {
                X = rand() % (board[0].sizeX - 2) + 2;
                Y = rand() % (board[0].sizeY - 2) + 2;
            } else {
                for (int attempt = 1; attempt <= 30; attempt++) {
                    int rd = rand() % found_body.size();
                    int num = rd;
                    for(int i = 1; i <= board[0].sizeX; i++) {
                        for(int j = 1; j <= board[0].sizeY; j++) {
                            if (board[0].map[i][j][0] > 0 && board[0].map[i][j][1] == 1 && num >= 0) {
                                X = i; Y = j; num--;
                            }
                        }
                    }
                    if (X == 1) X++;
                    if (Y == 1) Y++;
                    if (X == board[0].sizeX) X--;
                    if (Y == board[0].sizeY) Y--;
                    int exposed_count = 0;
                    bool found_head = false;
                    for (int i = -1; i <= 1; i++) {
                        for (int j = -1; j <= 1; j++) {
                            if (board[0].map[X + i][Y + j][0] > 0) {
                                exposed_count++;
                                if (board[0].map[X + i][Y + j][1] == 2) found_head = true;
                            }
                        }
                    }
                    if (!found_head && exposed_count <= 7) break;
                    if ((exposed_count == 9 || found_head) && found_body.size() > 1) found_body.erase(found_body.begin() + rd);
                }
            }
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    board[0].Attack(X + i, Y + j);
                }
            }
            if (found_body.empty()) {
                return "【WARNING】BOSS发动技能 [热跟踪弹]！但未跟踪到任何飞机，导弹随机打击了 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 所在的 3*3 区域";
            } else {
                return "【WARNING】BOSS发动技能 [热跟踪弹]！导弹跟踪了位于 " + (string(1, 'A' + X - 1) + to_string(Y)) + " 的机身，并打击了附近的 3*3 区域";
            }
        }
        else if (skill > 100 - skill_probability[2] - add_p * 3)
        {
            // 15%概率发动自爆无人机，随机打击3个十字区域
            int success = 0;
            const int num = board[1].alive <= board[1].planeNum * board[1].crucial_num - 3 * board[1].crucial_num ? 2 : 3;
            string str[3], areas;
            while (success < num) {
                X = rand() % (board[0].sizeX - 4) + 3;
                Y = rand() % (board[0].sizeY - 4) + 3;
                str[success] = string(1, 'A' + X - 1) + to_string(Y);
                if (success == 1 && str[success] == str[0]) continue;
                if (success == 2 && (str[success] == str[0] || str[success] == str[1])) continue;
                // 十字形同BOSS2研发中心
                board[0].Attack(X, Y);
                for (auto position : RDcenter_position) {
                    board[0].Attack(X + position.first, Y + position.second);
                }
                success++;
                areas += (string(1, 'A' + X - 1) + to_string(Y)) + " ";
            }
            return "【WARNING】BOSS发动技能 [自爆无人机]！随机打击了地图上位于 " + areas + "的 " + to_string(num) + " 个十字形区域";
        }
        else if (skill > 100 - skill_probability[3] - add_p * 4)
        {
            // 15%概率发动分导弹头，每发导弹落点5*5内随机打击3个格子
            vector<vector<int>> normal_attack;
            for(int i = 1; i <= board[0].sizeX; i++) {
                for(int j = 1; j <= board[0].sizeY; j++) {
                    if (board[0].this_turn[i][j] == 1) {
                        normal_attack.push_back(vector<int>{i, j});
                    }
                }
            }
            string areas;
            int count = 0;
            int try_count, temp_count;
            for(int i = 0; i < normal_attack.size(); i++) {
                try_count = temp_count = 0;
                while (temp_count < 3 && try_count++ < 1000) {
                    X = normal_attack[i][0] + rand() % 5 - 2;
                    Y = normal_attack[i][1] + rand() % 5 - 2;
                    string ret = board[0].Attack(X, Y);
                    if (ret == "0" || ret == "1" || ret == "2") {
                        temp_count++;
                        count++;
                    }
                }
                areas += (string(1, 'A' + X - 1) + to_string(Y)) + " ";
            }
            return "【WARNING】BOSS发动技能 [分导弹头]！在导弹落点 " + areas + "附近的5*5区域内进行分散打击，总计额外打击了 " + to_string(count) + " 个位置";
        }
        else if (skill > 100 - skill_probability[4] && board[1].alive > board[1].planeNum * board[1].crucial_num - 3 * board[1].crucial_num)
        {
            // 15%概率发动电磁干扰，玩家导弹在5*5区域偏移
            EMI = true;
            return "【WARNING】BOSS发动技能 [电磁干扰]！本回合玩家的所有导弹受到干扰，实际落点会在选择的坐标周围 5*5 区域内发生偏移";
        }
        return "";
    }


    // BOSS 被动技能
    pair<string, string> BOSS_PassiveSkills(Board (&board)[2], string str) const
    {
        // BOSS2 [导弹拦截]
        if (BossType == 2 || tempBossType == 2) {
            if (rand() % 100 < 8) {
                string ret = board[0].PlayerAttack(str);
                if (ret == "0" || ret == "1" || ret == "2") {
                    return make_pair(ret, "【WARNING】BOSS触发技能 [导弹拦截]，当前导弹被拦截并打击到了玩家的地图上");
                }
            }
        }
        // BOSS3 [电磁干扰]
        if (BossType == 3 || tempBossType == 3) {
            if (EMI) {
                if (Board::CheckCoordinate(str) != "OK") return make_pair("Failed", "Empty");
                int X = str[0] - 'A' + 1, Y = str[1] - '0';
                if (str.length() == 3) Y = (str[1] - '0') * 10 + str[2] - '0';
                if (board[1].map[X][Y][0] > 0) return make_pair("Failed", "Empty");

                int try_count = 0;
                while (try_count++ < 1000) {
                    int actual_X = X + rand() % 5 - 2;
                    int actual_Y = Y + rand() % 5 - 2;
                    string actual_str = string(1, 'A' + actual_X - 1) + to_string(actual_Y);
                    string ret = board[1].PlayerAttack(actual_str);
                    if (ret == "0" || ret == "1" || ret == "2" || ret == "3") {
                        if (actual_str == str) {
                            return make_pair(ret, "虽然受到了强烈的电磁干扰，但导弹还是成功击中了 " + actual_str + "。\n令人振奋的消息...也许？");
                        } else {
                            return make_pair(ret, "受强烈电磁干扰的影响，导弹无法准确锁定位置，导弹击中了 " + actual_str);
                        }
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
        // BOSS0 万能核心被击中
        if (BossType == 0) {
            RD_is_hit = true;
            Board::CheckCoordinate(str);
            int X = str[0] - 'A' + 1, Y = str[1] - '0'; 
            if (str.length() == 3) {
                Y = (str[1] - '0') * 10 + str[2] - '0';
            }
            board[1].mark[X + 1][Y] = board[1].mark[X - 1][Y] = board[1].mark[X][Y + 1] = board[1].mark[X][Y - 1] = 300;
            return "万能核心被击中！BOSS所有形态的特殊技能均已被削弱。\n" + info;
        }
        // BOSS2 核弹研发中心被击中
        if (BossType == 2) {
            RD_is_hit = true;
            Board::CheckCoordinate(str);
            int X = str[0] - 'A' + 1, Y = str[1] - '0'; 
            if (str.length() == 3) {
                Y = (str[1] - '0') * 10 + str[2] - '0';
            }
            for (auto position : RDcenter_position) {
                board[1].mark[X + position.first][Y + position.second] = 300;
            }
            return "核弹研发中心被击中！研发成功概率已大幅下降。\n" + info;
        }
        return info;
    }

};


class Grid
{
  public:
    // 按钮触发位置关联
    struct ButtonTarget {
        int dx;
        int dy;
        std::optional<Direct> dir;
    };

    void PortalTeleport(Player& player) const
    {
        player.x += portalRelPos.first;
        player.y += portalRelPos.second;
        player.subspace = -1;
    }

    void TrapTrigger() { trap = !trap; }

    void switchDoor(const Direct dir)
    {
        Wall& target = wall[static_cast<int>(dir)];
        switch (target) {
            case Wall::DOOR:     target = Wall::DOOROPEN; break;
            case Wall::DOOROPEN: target = Wall::DOOR;     break;
            default:;
        }
    }

    void HideSpecialWalls()
    {
        for (int i = 0; i < 4; i++)
            if (CanPass(i))
                wall[i] = Wall::EMPTY;
            else
                wall[i] = Wall::NORMAL;
    }

    template <Direct direct>
    bool CanPass() const { return CanPass(static_cast<int>(direct)); }
    bool CanPass(const int wall_id) const
    {
        return wall[wall_id] == Wall::EMPTY || wall[wall_id] == Wall::DOOROPEN;
    }
    bool IsFullyEnclosed() const
    {
        return wall[0] == Wall::NORMAL && wall[1] == Wall::NORMAL && wall[2] == Wall::NORMAL && wall[3] == Wall::NORMAL;
    }
    bool ContainWallType(const Wall w) const
    {
        return wall[0] == w || wall[1] == w || wall[2] == w || wall[3] == w;
    }
    bool HasBox() const
    {
        switch (attach) {
            case AttachType::BOX:       return true;
            case AttachType::HEATBOX:   return true;
            case AttachType::JAMMERBOX: return true;
            default: return false;
        }
    }

    template <Direct direct>
    void SetWall(const Wall new_wall) { wall[static_cast<int>(direct)] = new_wall; }
    void SetWall(Direct direct, const Wall new_wall) { wall[static_cast<int>(direct)] = new_wall; }
	Grid& SetWall(const Wall up, const Wall down, const Wall left, const Wall right)
    {
        wall[0] = up;
        wall[1] = down;
        wall[2] = left;
        wall[3] = right;
        return *this;
    }
    Grid& SetType(const GridType type)
    {
        this->grid = type;
        return *this;
    }
    Grid& SetAttach(const AttachType type)
    {
        this->attach = type;
        return *this;
    }
    Grid& SetPortal(const int relPosX, const int relPosY) {
        this->portalRelPos = {relPosX, relPosY};
        return *this;
    }
    Grid& SetButton(const vector<ButtonTarget>& button_targets)
    {
        this->buttonTargetPos = button_targets;
        return *this;
    }
    void SetGrowable(const bool growable) { this->growable = growable; }
    void SetContent(const string& content) { this->content.first = content; }
    void SetWallContent(const vector<string>& wall_content) { this->content.second = wall_content; }

    template <Direct direct>
    Wall GetWall() const { return wall[static_cast<int>(direct)]; }
    Wall GetWall(Direct direct) const { return wall[static_cast<int>(direct)]; }
    GridType Type() const { return grid; }
    AttachType Attach() const { return attach; }
    pair<int, int> PortalPos() const { return portalRelPos; }
    bool TrapStatus() const { return trap; }
    bool CanGrow() const { return growable; }
    pair<string, vector<string>> GetContent() const { return content; }
    vector<ButtonTarget> ButtonTargetPos() const { return buttonTargetPos; }

    // 获取墙壁上的提示文本
    static string GetWallContent(const vector<vector<Grid>>& grid_map, const int x, const int y, const Direct direction)
    {
        const int d = static_cast<int>(direction);
        const int od = static_cast<int>(opposite(direction));
        const vector<string> vec1 = grid_map[x][y].GetContent().second;
        if (d >= 0 && d < (int)vec1.size() && !vec1[d].empty()) {
            return vec1[d];
        }
        int nx = x + k_DX_Direct[d];
        int ny = y + k_DY_Direct[d];
        if (0 <= nx && nx < grid_map.size() && 0 <= ny && ny < grid_map.size()) {
            const vector<string> vec2 = grid_map[nx][ny].GetContent().second;
            if (od >= 0 && od < (int)vec2.size() && !vec2[od].empty()) {
                return vec2[od];
            }
        }
        return "";
    }

  private:
    // 区块类型
	GridType grid = GridType::EMPTY;
    // 附着类型
    AttachType attach = AttachType::EMPTY;
	// 四周墙面（上/下/左/右）
	Wall wall[4] = { Wall::EMPTY, Wall::EMPTY, Wall::EMPTY, Wall::EMPTY };

    // 预览用提示文本
    pair<string, vector<string>> content = {"", {}};
    // 特殊规则2草丛是否可生长
    bool growable = false;

    // 传送门相对位置（PORTAL）
    pair<int, int> portalRelPos = {0, 0};
    // 按钮触发位置（BUTTON）
    vector<ButtonTarget> buttonTargetPos;

    // 陷阱状态（TRAP）
    bool trap = true;
};

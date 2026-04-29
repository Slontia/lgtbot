#pragma once

#include <map>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

enum AnimalType
{
    Bird,
    Herbivorous,
    Carnivorous,
};

const std::vector<std::string> AllAnimals = {"母鸡", "小鸡", "鸭鸭", "天鹅", "火鸡", "猪猪", "牛牛", "山羊", "袋鼠", "狗狗", "狐狸", "灰狼", "老虎"};

std::map<std::string, AnimalType> AnimalTypeMap =
{
    {"母鸡", Bird},
    {"小鸡", Bird},
    {"鸭鸭", Bird},
    {"天鹅", Bird},
    {"火鸡", Bird},
    {"猪猪", Herbivorous},
    {"牛牛", Herbivorous},
    {"山羊", Herbivorous},
    {"袋鼠", Herbivorous},
    {"狗狗", Carnivorous},
    {"狐狸", Carnivorous},
    {"灰狼", Carnivorous},
    {"老虎", Carnivorous},
};
std::map<std::string, int> AnimalScoreMap =
{
    {"母鸡", 2},
    {"小鸡", 2},
    {"鸭鸭", 1},
    {"天鹅", 1},
    {"火鸡", 1},
    {"猪猪", 2},
    {"牛牛", 1},
    {"山羊", 1},
    {"袋鼠", 2},
    {"狗狗", 1},
    {"狐狸", 1},
    {"灰狼", 1},
    {"老虎", 1},
};

class Pasture
{
public:
    Pasture() {}

    std::vector<std::string> ShuffleN()
    {
        std::vector<std::string> animals = {"母鸡", "鸭鸭", "天鹅", "火鸡", "猪猪", "牛牛", "山羊", "袋鼠", "狗狗", "狐狸", "灰狼", "老虎"};
        std::random_device rd;
        std::uniform_int_distribution<unsigned long long> dis;
        std::string seed_str = std::to_string(dis(rd));
        std::seed_seq seed(seed_str.begin(), seed_str.end());
        std::mt19937 g(seed);
        std::shuffle(animals.begin(), animals.end(), g);
        if (++mShuffleTime <= 3) {
            return std::vector<std::string>(animals.begin(), animals.begin() + 6);
        }
        return std::vector<std::string>(animals.begin(), animals.begin() + 3);
    }

    std::map<std::string, int> GetAnimal() { return mAnimal; }

    void AddAnimal(const std::string& name) { mAnimal[name] += 1; }

    void RemoveAnimal(std::string name)
    {
        if (mAnimal.count(name) && mAnimal[name] >= 1) {
            mAnimal[name] -= 1;
        }
    }
    
    std::vector<std::string> GetGrazing() { return mGrazing; }
    
    int GetBuyCount() { return (mShuffleTime <= 3) ? 2: (1 + mAddBuy) <= 3 ? 1 + mAddBuy : 3; }

    int GetScore() const { return mScore; }

    int GetRemoveCount() { 
        int ret = 0;
        for (int i = 0; i < 3; i++)
        {
            if (mGrazing[i] == "袋鼠")
            {
                ret++;
            }
        }
        return (ret < All().size() - mGrazing.size()) ? ret :  All().size() - mGrazing.size();
    }

    std::vector<std::string> All()
    {
        std::vector<std::string> animals;
        for (auto iter: mAnimal)
        {
            for (int i = 0; i < iter.second; i++)
            {
                animals.emplace_back(iter.first);
            }
        }
        return animals;
    }

    std::map<std::string, int> Rest() { 
        std::map<std::string, int> mRest = mAnimal;
        for (auto name: mGrazing)
        {
            mRest[name] -= 1;
        } 
        return mRest;
    }

    void Rand()
    {
        // 抽卡
        std::vector<std::string> animals = All();
        std::random_device rd;
        std::uniform_int_distribution<unsigned long long> dis;
        std::string seed_str = std::to_string(dis(rd));
        std::seed_seq seed(seed_str.begin(), seed_str.end());
        std::mt19937 g(seed);
        std::shuffle(animals.begin(), animals.end(), g);
        if (animals.size() > 3) 
        {
            mGrazing = std::vector<std::string>(animals.begin(), animals.begin() + 3);
        }
        else
        {
            mGrazing = animals;
        }
    }

    std::string Grazing()
    {
        std::string ret = "";

        mAddBuy = 0;

        ret += "原分数：" + std::to_string(mScore) + "\n";

        std::vector<int> BeEat(mGrazing.size(), 0);

        int s;
        int birdCount = 0;      // 鸟类数量
        int goatAddCount = 0;   // 山羊得分数量
        int typeCount = 0;      // 几种动物
        int helpType[3] = {0};  // 帮助计算几种动物
        int tigerCount = 0;     // 老虎数量
        
        for (int i = 0; i < mGrazing.size(); i++)
        {
            AnimalType t = AnimalTypeMap[mGrazing[i]];
            if (t == Bird)
            {
                birdCount++;
            }
            if (mGrazing[i] != "山羊" && (t == Bird || t == Herbivorous))
            {
                goatAddCount++;
            }
            helpType[t]++;
            if (mGrazing[i] == "老虎")
            {
                tigerCount++;
            }
        }
        
        for (int i = 0; i < 3; i++)
        {
            if (helpType[i])
            {
                typeCount++;
            }
        }

        for (int i = 0; i < mGrazing.size(); i++)
        {
            s = AnimalScoreMap[mGrazing[i]];
            mScore += s;
            ret += std::to_string(i+1) + "号" + mGrazing[i] + "基础得分" + std::to_string(s) + "，当前得分：" + std::to_string(mScore)+  "\n";
            if (mGrazing[i] == "鸭鸭")
            {
                if (birdCount == 2 || birdCount == 3)
                {
                    s = birdCount;
                    mScore += s;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "额外得分" + std::to_string(s) + "，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
            else if (mGrazing[i] == "天鹅")
            {
                if (birdCount == 1)
                {
                    s = 2;
                    mScore += s;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "额外得分" + std::to_string(s) + "，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
            else if (mGrazing[i] == "山羊")
            {
                if (goatAddCount)
                {
                    s = 2 * goatAddCount;
                    mScore += s;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "额外得分" + std::to_string(s) + "，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
            else if (mGrazing[i] == "狗狗")
            {
                if (typeCount == 2 || typeCount == 3)
                {
                    s = typeCount;
                    mScore += s;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "额外得分" + std::to_string(s) + "，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
        }

        if (tigerCount > 1)
        {
            for (int i = 0; i < mGrazing.size(); i++)
            {
                if (mGrazing[i] == "老虎")
                {
                    BeEat[i] = 1;
                }
            }
            s = tigerCount * 5;
            mScore += s;
            ret += std::to_string(tigerCount) + "只老虎互吃，得" + std::to_string(s) + "分，当前得分：" + std::to_string(mScore)+  "\n";
        }
        else
        {
            for (int i = 0; i < mGrazing.size(); i++)
            {
                if (mGrazing[i] == "老虎")
                {
                    for (int j = 0; j < mGrazing.size() && !BeEat[j]; j++)
                    {
                        if (AnimalTypeMap[mGrazing[j]] == Carnivorous && mGrazing[j] != "老虎")
                        {
                            BeEat[j] = 1;
                            s = 5;
                            mScore += s;
                            ret += std::to_string(i+1) + "号" + mGrazing[i] + "吃掉了" + std::to_string(j+1) + "号" + mGrazing[j] + "，得" + std::to_string(s) + "分，当前得分：" + std::to_string(mScore)+  "\n";
                            break;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < mGrazing.size(); i++)
        {
            if (!BeEat[i] && mGrazing[i] == "狐狸")
            {
                for (int j = 0; j < mGrazing.size() && !BeEat[j]; j++)
                {
                    if (AnimalTypeMap[mGrazing[j]] == Bird)
                    {
                        BeEat[j] = 1;
                        s = 5;
                        mScore += s;
                        ret += std::to_string(i+1) + "号" + mGrazing[i] + "吃掉了" + std::to_string(j+1) + "号" + mGrazing[j] + "，得" + std::to_string(s) + "分，当前得分：" + std::to_string(mScore)+  "\n";
                        break;
                    }
                }
            }
        }
        for (int i = 0; i < mGrazing.size(); i++)
        {
            if (!BeEat[i] && mGrazing[i] == "灰狼")
            {
                for (int j = 0; j < mGrazing.size() && !BeEat[j]; j++)
                {
                    if (AnimalTypeMap[mGrazing[j]] == Herbivorous)
                    {
                        BeEat[j] = 1;
                        s = 5;
                        mScore += s;
                        ret += std::to_string(i+1) + "号" + mGrazing[i] + "吃掉了" + std::to_string(j+1) + "号" + mGrazing[j] + "，得" + std::to_string(s) + "分，当前得分：" + std::to_string(mScore)+  "\n";
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < mGrazing.size(); i++)
        {
            if (BeEat[i])
            {
                if (mGrazing[i] == "母鸡")
                {
                    AddAnimal("小鸡");
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "被吃，添加了一只小鸡\n";
                }
                else if (mGrazing[i] == "火鸡")
                {
                    s = 5;
                    mScore += s;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "被吃，得" + std::to_string(s) + "分，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
            else
            {
                if (mGrazing[i] == "猪猪")
                {
                    AddAnimal("猪猪");
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "没被吃，添加了一只猪猪\n";
                }
                else if (mGrazing[i] == "牛牛")
                {
                    RemoveAnimal("牛牛");
                    s = 2;
                    mScore += 2;
                    mAddBuy += 1;
                    ret += std::to_string(i+1) + "号" + mGrazing[i] + "没被吃，得" + std::to_string(s) + "分并移除，下回合选卡+1，当前得分：" + std::to_string(mScore)+  "\n";
                }
            }
        }
        
        for (int i = 0; i < mGrazing.size(); i++)
        {
            if (BeEat[i])
            {
                RemoveAnimal(mGrazing[i]);
            }
        }
        return ret;
    }

private:
    std::map<std::string, int> mAnimal;
    std::vector<std::string> mGrazing;
    int mShuffleTime = 0;
    int mAddBuy = 0;
    int mScore = 0;
};

std::string ToString(std::map<std::string, int> mAnimal)
{
    std::string ret = "";
    for (const std::string& name: AllAnimals) {
        if (mAnimal.count(name) && mAnimal[name] >= 1) {
            ret += name;
            if (mAnimal[name] > 1) {
                ret += "×" + std::to_string(mAnimal[name]);
            }
            ret += " ";
        }
    }
    return ret;
}

std::string ToString(std::vector<std::string> mGrazing)
{
    std::string ret = "";
    for (const std::string& name: mGrazing) {
        ret += name +" ";
    }
    return ret;
}

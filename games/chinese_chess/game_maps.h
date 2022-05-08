#include "game_util/laser_chess.h"

inline laser::Board InitAceBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 4}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 5}, KingChess<0>());
    b.SetChess(Coor{0, 6}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 7}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{1, 2}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{2, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{3, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{3, 2}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{3, 4}, DoubleMirrorChess<0>(false));
    b.SetChess(Coor{3, 5}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{3, 7}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{3, 9}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{4, 0}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{4, 2}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{4, 4}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{4, 5}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{4, 7}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{4, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{5, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{6, 7}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{7, 2}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{7, 3}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 4}, KingChess<1>());
    b.SetChess(Coor{7, 5}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitCuriosityBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 4}, ShieldChess<0>(LEFT));
    b.SetChess(Coor{0, 5}, KingChess<0>());
    b.SetChess(Coor{0, 6}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 7}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{2, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{2, 6}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{3, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{3, 1}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{3, 4}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{3, 5}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{3, 8}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{3, 9}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{4, 0}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{4, 1}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{4, 4}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{4, 5}, SingleMirrorChess<0>(DOWN));
    b.SetChess(Coor{4, 8}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{4, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{5, 3}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{5, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{7, 2}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{7, 3}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 4}, KingChess<1>());
    b.SetChess(Coor{7, 5}, ShieldChess<1>(RIGHT));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(LEFT, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitGrailBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 4}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{0, 5}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{1, 5}, KingChess<0>());
    b.SetChess(Coor{2, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{2, 4}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{2, 5}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{2, 6}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{3, 0}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{3, 2}, DoubleMirrorChess<0>(false));
    b.SetChess(Coor{3, 4}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{3, 6}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{4, 3}, SingleMirrorChess<0>(DOWN));
    b.SetChess(Coor{4, 5}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{4, 7}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{4, 9}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{5, 3}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{5, 4}, ShieldChess<1>(UP));
    b.SetChess(Coor{5, 5}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{5, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{6, 4}, KingChess<1>());
    b.SetChess(Coor{7, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{7, 4}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 5}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitMercuryBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 4}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{0, 5}, KingChess<0>());
    b.SetChess(Coor{0, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{0, 9}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{1, 5}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{1, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{2, 0}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{2, 3}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{2, 5}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{3, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{3, 4}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{3, 8}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{4, 1}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{4, 5}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{4, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{5, 4}, ShieldChess<1>(UP));
    b.SetChess(Coor{5, 6}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{5, 9}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{6, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{6, 4}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 0}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{7, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{7, 4}, KingChess<1>());
    b.SetChess(Coor{7, 5}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(LEFT, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitSophieBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 4}, KingChess<0>());
    b.SetChess(Coor{0, 5}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{0, 6}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{1, 3}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{1, 5}, ShieldChess<0>(RIGHT));
    b.SetChess(Coor{1, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{2, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{2, 4}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{2, 5}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{2, 7}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{2, 9}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{3, 7}, DoubleMirrorChess<0>(false));
    b.SetChess(Coor{4, 2}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{5, 0}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{5, 2}, DoubleMirrorChess<0>(true));
    b.SetChess(Coor{5, 4}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{5, 5}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{5, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{6, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{6, 4}, ShieldChess<1>(LEFT));
    b.SetChess(Coor{6, 6}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{7, 4}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{7, 5}, KingChess<1>());
    b.SetChess(Coor{7, 9}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitGeniusBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 8, std::move(image_path));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 1}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{0, 2}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 3}, KingChess<0>());
    b.SetChess(Coor{0, 4}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{0, 5}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{0, 7}, LensedMirrorChess<1>(true));
    b.SetChess(Coor{3, 3}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{3, 4}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{3, 7}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{4, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{4, 3}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{4, 4}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{7, 0}, LensedMirrorChess<0>(true));
    b.SetChess(Coor{7, 2}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{7, 3}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 4}, KingChess<1>());
    b.SetChess(Coor{7, 5}, ShieldChess<1>(UP));
    b.SetChess(Coor{7, 6}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{7, 7}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    return b;
}

inline laser::Board InitRefractionBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{1, 9}, DoubleMirrorChess<0>(false));
    b.SetChess(Coor{4, 9}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{5, 9}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(UP, std::bitset<4>().set(UP).set(LEFT)));
    b.SetChess(Coor{1, 7}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{5, 7}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{1, 6}, LensedMirrorChess<0>(true));
    b.SetChess(Coor{3, 6}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{4, 6}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{6, 6}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{1, 5}, KingChess<0>());
    b.SetChess(Coor{2, 5}, ShieldChess<0>(LEFT));
    b.SetChess(Coor{6, 5}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 4}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{5, 4}, ShieldChess<1>(RIGHT));
    b.SetChess(Coor{6, 4}, KingChess<1>());
    b.SetChess(Coor{1, 3}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{3, 3}, SingleMirrorChess<0>(DOWN));
    b.SetChess(Coor{4, 3}, SingleMirrorChess<0>(DOWN));
    b.SetChess(Coor{6, 3}, LensedMirrorChess<1>(true));
    b.SetChess(Coor{2, 2}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{6, 2}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(DOWN).set(RIGHT)));
    b.SetChess(Coor{2, 0}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{3, 0}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{6, 0}, DoubleMirrorChess<1>(false));

    return b;
}

inline laser::Board InitGeminiBoard(std::string image_path)
{
    using namespace laser;
    Board b(8, 10, std::move(image_path));
    b.SetChess(Coor{0, 9}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 9}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{7, 9}, ShooterChess<1>(UP, std::bitset<4>().set(UP).set(LEFT)));
    b.SetChess(Coor{4, 8}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{6, 8}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{7, 8}, SingleMirrorChess<1>(LEFT));
    b.SetChess(Coor{3, 7}, SingleMirrorChess<0>(UP));
    b.SetChess(Coor{4, 7}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{7, 7}, ShieldChess<1>(UP));
    b.SetChess(Coor{1, 6}, DoubleMirrorChess<0>(false));
    b.SetChess(Coor{7, 6}, SingleMirrorChess<0>(DOWN));
    b.SetChess(Coor{3, 5}, KingChess<0>());
    b.SetChess(Coor{4, 5}, ShieldChess<0>(RIGHT));
    b.SetChess(Coor{3, 4}, ShieldChess<1>(LEFT));
    b.SetChess(Coor{4, 4}, KingChess<1>());
    b.SetChess(Coor{0, 3}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{6, 3}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{0, 2}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{3, 2}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{4, 2}, SingleMirrorChess<1>(DOWN));
    b.SetChess(Coor{0, 1}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{1, 1}, SingleMirrorChess<0>(RIGHT));
    b.SetChess(Coor{3, 1}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{0, 0}, ShooterChess<0>(DOWN, std::bitset<4>().set(DOWN).set(RIGHT)));
    b.SetChess(Coor{6, 0}, SingleMirrorChess<0>(LEFT));
    b.SetChess(Coor{7, 0}, LensedMirrorChess<0>(false));

    return b;
}

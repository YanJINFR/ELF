/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "engine/cmd.h"
#include "engine/game_env.h"
#include "engine/cmd_receiver.h"

// Derived class. Note that the definition is automatically generated by a python file.
#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "lua/cpp_interface.h"

bool CmdGenerateMap::run(GameEnv *env, CmdReceiver*) {
    RTSMapGenerator::Generate(env->GetMap(), env->GetNumOfPlayers(), _seed);
    // create fog for each player
    for (int player_id = 0; player_id < env->GetNumOfPlayers(); ++player_id) {
        env->GetPlayer(player_id).CreateFog();
    }

    return true;
}

#define _CREATE(...) receiver->SendCmd(CmdIPtr(new CmdCreate(INVALID, __VA_ARGS__)))
#define _CHANGE_RES(...) receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(INVALID, __VA_ARGS__)))


bool CmdGameStartSpecific::run(GameEnv* env, CmdReceiver* receiver) {
    RTSUnitGenerator::Generate(env->GetMap(), *receiver, env->GetNumOfPlayers(), 0);
    return true;
}

bool CmdGenerateUnit::run(GameEnv *env, CmdReceiver *receiver) {
    auto f = env->GetRandomFunc();
    int lr_seed = f(2);
    int ud_seed = f(2);
    bool shuffle_lr = (lr_seed == 0);
    bool shuffle_ud = (ud_seed == 0);
    auto shuffle_loc = [&] (PointF p, bool b1, bool b2) -> PointF {
        int x = b1 ? env->GetMap().GetXSize() - 1 - p.x : p.x;
        int y = b2 ? env->GetMap().GetYSize() - 1 - p.y : p.y;
        return PointF(x, y);
    };

    receiver->GetGameStats().PickBase(lr_seed * 2 + ud_seed);
    for (const auto &info : env->GetMap().GetPlayerMapInfo()) {
        PlayerId id = info.player_id;
        _CREATE(BASE, shuffle_loc(PointF(info.base_coord), shuffle_lr, shuffle_ud), id);
        _CREATE(RESOURCE, shuffle_loc(PointF(info.resource_coord), shuffle_lr, shuffle_ud), id);
        _CHANGE_RES(id, info.initial_resource);
        //base_locs[id] = PointF(info.base_coord);
    }
    // for (size_t i = 0; i < base_locs.size(); ++i) {
    //    std::cout << "[" << i << "] Baseloc: " << base_locs[i].x << ", " << base_locs[i].y << std::endl;
    //}
    auto gen_loc = [&] (int player_id) -> PointF {
        // Note that we could not write
        //    PointF( f(8) + ...,  f(8) + ...)
        // since the result will depend on which f is evaluated first, and will yield different results on
        // different platform/compiler (e.g., clang and gcc yields different results).
        // The following implementation is uniquely determined.
        int x = f(6) + player_id * 10 + 2;
        int y = f(6) + player_id * 10 + 2;
        return PointF(x, y);
    };
    for (PlayerId player_id = 0; player_id < 2; player_id++) {
        PlayerId id = player_id;
        // Generate workers (up to three).
        for (int i = 0; i < 3; i++) {
            if (f(10) >= 5) {
                _CREATE(WORKER, shuffle_loc(gen_loc(player_id), shuffle_lr, shuffle_ud), id);
            }
        }
        if (f(10) >= 8)
            _CREATE(BARRACKS, shuffle_loc(gen_loc(player_id), shuffle_lr, shuffle_ud), id);
        if (f(10) >= 5)
            _CREATE(MELEE_ATTACKER, shuffle_loc(gen_loc(player_id), shuffle_lr, shuffle_ud), id);
        if (f(10) >= 5)
            _CREATE(RANGE_ATTACKER, shuffle_loc(gen_loc(player_id), shuffle_lr, shuffle_ud), id);
    }
    return true;
}

#undef _CHANGE_RES
#undef _CREATE

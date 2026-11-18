#pragma once
#include "legacy10_codec.h"

enum { VB_RIDE_EPONA, VB_PLANT_MAGIC_BEAN, ABILITY_HOOK_COUNT };
inline void (*abilityHooks[ABILITY_HOOK_COUNT])(bool*) = {};
inline bool isRando = true;
#define IS_RANDO isRando
#define COND_VB_SHOULD(hook, condition, body) abilityHooks[hook] = condition ? +[](bool* should) body : nullptr
inline bool GameInteractor_Should(int hook, bool result) {
    if (abilityHooks[hook]) {
        abilityHooks[hook](&result);
    }
    return result;
}
#include "Rando/MiscBehavior/Abilities.cpp"
#undef COND_VB_SHOULD
#undef IS_RANDO

enum { ACTOR_EN_HORSE, ACTOR_OBJ_AQUA };
enum { ENHORSE_1, ENHORSE_2, ENHORSE_7, ENHORSE_11, ENHORSE_13, ENHORSE_PARAM_4000 = 0x4000 };
enum { PLAYER_FORM_HUMAN, PLAYER_STATE1_800000 = 0x800000 };
enum { WEEKEVENTREG_HORSE_RACE_STATE_START = 1, WEEKEVENTREG_HORSE_RACE_STATE_2, WEEKEVENTREG_HORSE_RACE_STATE_3 };
inline int GET_WEEKEVENTREG_HORSE_RACE_STATE = 0;
#define SET_WEEKEVENTREG_HORSE_RACE_STATE(value) (GET_WEEKEVENTREG_HORSE_RACE_STATE = (value))
enum { NA_BGM_HORSE = 1, NA_BGM_GENERAL_SFX, TRANS_TYPE_FADE_WHITE_FAST, TRANS_TRIGGER_START };
inline void Environment_ForcePlaySequence(int) {}
inline int D_801BDAA0 = 0;
inline int spawnedHorse = -1;
inline Actor horse;
using CollisionPoly = int;
constexpr float BGCHECK_Y_MIN = -32000;
#define ENHORSE_PARAMS(flags, type) ((flags) | (type))
inline Actor* Actor_Spawn(int*, PlayState*, int, float, float, float, s16, s16, s16, int params) {
    spawnedHorse = params & ~ENHORSE_PARAM_4000;
    horse = {};
    return &horse;
}
inline float BgCheck_EntityRaycastFloor1(int*, CollisionPoly**, Vec3f* pos) { return pos->y; }
inline bool Horse_IsValidSceneLayer(PlayState*, Player*) { return gSaveContext.sceneLayer == 0; }
inline bool Horse_IsValidSpawn(s16) { return true; }
inline void Horse_ResetHorseData(PlayState*) { gSaveContext.save.saveInfo.horseData = {}; }
inline int Cutscene_GetSceneLayer(PlayState*) { return gSaveContext.sceneLayer; }
inline void Player_SetCameraHorseSetting(PlayState*, Player*) {}

struct EnIn { Actor actor; unsigned unk4AC = 0; };
constexpr unsigned WEEKEVENTREG_57_01 = 32;
constexpr int NA_SE_SY_ERROR = 0;
inline int raceText = 0;
inline bool CHECK_WEEKEVENTREG(unsigned flag) { return (flags & flag) != 0; }
inline void Rupees_ChangeBy(int delta) { gSaveContext.save.saveInfo.playerData.rupees += delta; }
inline void Actor_ContinueText(PlayState*, Actor*, int text) { raceText = text; }
inline void func_808F4108(EnIn*, PlayState*, int text) { raceText = text; }
inline void Audio_PlaySfx_MessageDecide() {}
inline void Audio_PlaySfx_MessageCancel() {}
inline void Audio_PlaySfx(int) {}

enum { PLAYER_IA_MAGIC_BEANS = 1, AC_HIT = 1 };
struct ObjBean {
    struct { Actor actor; } dyna;
    struct { struct { int acFlags = 0; Actor* ac = nullptr; } base; } collider;
    void (*unk_1E8)(ObjBean*) = [](ObjBean*) {};
    int unk_1E4 = 0, unk_1DF = 0, unk_1FF = 1, unk_1FE = 0;
};
inline bool beanFlags[4]{};
inline bool beanOffered, beanAccepted, beanRain, beanPlayerOnTop, beanPlatformMoved;
inline int beanAmmo;
#define OBJBEAN_GET_SWITCH_FLAG_1(actor, index) (index)
#define OBJBEAN_GET_SWITCH_FLAG_2(actor, index) ((index) + 2)
inline void Flags_SetSwitch(PlayState*, int flag) { beanFlags[flag] = true; }
inline bool Flags_GetSwitch(PlayState*, int flag) { return beanFlags[flag]; }
inline bool Actor_TalkOfferAccepted(Actor*, int*) { return beanAccepted; }
inline int Player_GetExchangeItemAction(PlayState*) { return PLAYER_IA_MAGIC_BEANS; }
inline bool func_800FE9B4(PlayState*) { return beanRain; }
inline void Actor_OfferTalkExchangeEquiCylinder(Actor*, PlayState*, float, int) { beanOffered = true; }
inline void CollisionCheck_SetAC(PlayState*, int*, void*) {}
inline void func_809383B4(ObjBean*) {}
inline void func_809381B0(ObjBean*) {}
inline void func_809375C8(ObjBean*, PlayState*) {}
inline void func_80938670(ObjBean*) {}
template <class Dyna> bool DynaPolyActor_IsPlayerOnTop(Dyna*) { return beanPlayerOnTop; }
inline void func_80938874(ObjBean*) { beanPlatformMoved = true; }
inline void func_80936F24(ObjBean*) {}

#include "game_ability_functions.h"

inline void TestAbilityActions() {
    using Rando::Abilities::FindByItem;
    std::fill(std::begin(options), std::end(options), 0);
    std::fill(std::begin(randoInf), std::end(randoInf), false);
    const auto* ride = FindByItem(RI_ABILITY_RIDE_EPONA);
    const auto* plant = FindByItem(RI_ABILITY_PLANT_BEANS);
    assert(ride && plant && !FindByItem(RI_MAGIC_BEAN));

    int hookCases = 0;
    for (bool rando : { false, true }) {
        isRando = rando;
        Rando::Abilities::RegisterHooks();
        for (bool shuffle : { false, true }) {
            options[ride->option] = options[plant->option] = shuffle;
            for (bool obtained : { false, true }) {
                randoInf[ride->flag] = randoInf[plant->flag] = obtained;
                for (int hook : { VB_RIDE_EPONA, VB_PLANT_MAGIC_BEAN }) {
                    assert(GameInteractor_Should(hook, true) == (!rando || !shuffle || obtained));
                    assert(!GameInteractor_Should(hook, false));
                    hookCases += 2;
                }
            }
        }
    }

    // Both scripted ranch layers can mount. On completion, failure or exit into the overworld,
    // the transition can carry the mounted flag and the player can already know the song.
    randoInf[ride->flag] = false;
    hasEponasSong = true;
    int ranchCases = 0;
    const std::pair<u16, s16> returns[] = {
        { ENTRANCE(ROMANI_RANCH, 1), SCENE_F01 },
        { ENTRANCE(MILK_ROAD, 1), SCENE_ROMANYMAE },
        { ENTRANCE(TERMINA_FIELD, 0), SCENE_00KEIKOKU },
    };
    for (int layer : { 1, 5 }) {
        for (auto [destination, scene] : returns) {
            gSaveContext = {};
            PlayState ranch{};
            ranch.sceneId = SCENE_F01;
            Player player{};
            gSaveContext.save.entrance = ENTRANCE(ROMANI_RANCH, 0);
            gSaveContext.sceneLayer = layer;
            Horse_Spawn(&ranch, &player);
            assert(player.rideActor == &horse && (player.stateFlags1 & PLAYER_STATE1_800000));
            assert(spawnedHorse == ENHORSE_7);

            gHorseIsMounted = true; // z_player.c sets this on a horseback scene transition
            gSaveContext.save.entrance = destination;
            gSaveContext.sceneLayer = 0;
            PlayState next{};
            next.sceneId = scene;
            Player arriving{};
            Horse_Spawn(&next, &arriving);
            assert(!gHorseIsMounted && arriving.rideActor == nullptr && !(arriving.stateFlags1 & PLAYER_STATE1_800000));

            randoInf[ride->flag] = true;
            gHorseIsMounted = true;
            Horse_Spawn(&next, &arriving);
            assert(arriving.rideActor == &horse && spawnedHorse == ENHORSE_11);
            randoInf[ride->flag] = false;
            ++ranchCases;
        }
    }

    int raceCases = 0;
    for (int paymentPath : { 0, 1, 2 }) {
        for (bool skipRace : { false, true }) {
            for (bool unlocked : { false, true }) {
                randoInf[ride->flag] = unlocked;
                EnIn brother{};
                PlayState race{};
                race.msgCtx.unk1206C = 10;
                gSaveContext.save.saveInfo.playerData.rupees = 20;
                flags = WEEKEVENTREG_57_01;
                gSaveContext.save.entrance = ENTRANCE(GORMAN_TRACK, 0);
                GET_WEEKEVENTREG_HORSE_RACE_STATE = 0;
                if (paymentPath == 0) {
                    func_808F4150(&race, &brother, 2, &race.msgCtx);
                } else {
                    func_808F4270(&race, &brother, 7, &race.msgCtx, paymentPath == 2);
                }
                assert(raceText == (unlocked ? 0x3475 : 0x3472));
                assert(gSaveContext.save.saveInfo.playerData.rupees == (unlocked ? 10 : 20));
                if (raceText == 0x3475) {
                    StartGormanRace(&race);
                    gSaveContext.save.entrance = race.nextEntrance;
                }
                if (skipRace) {
                    SkipGormanRace();
                }
                assert(gSaveContext.save.entrance == ENTRANCE(GORMAN_TRACK, !unlocked ? 0 : (skipRace ? 2 : 5)));
                assert(race.transitionTrigger == (unlocked ? TRANS_TRIGGER_START : 0));
                ++raceCases;
            }
        }
    }

    for (bool unlocked : { false, true }) {
        randoInf[plant->flag] = unlocked;
        PlayState field{};
        ObjBean patch{};
        beanOffered = beanAccepted = false;
        beanAmmo = 1;
        func_80937FC8(&patch, &field);
        assert(beanOffered == unlocked);
        if (beanOffered) { // z_player.c can consume a bean only after the actor offers the exchange
            --beanAmmo;
            beanAccepted = true;
            func_80937FC8(&patch, &field);
        }
        assert(beanAmmo == !unlocked && beanFlags[2] == unlocked);
    }
    randoInf[plant->flag] = false;
    beanOffered = beanAccepted = false;
    int plantUses = 0;
    for (bool rain : { false, true }) {
        PlayState field{};
        ObjBean patch{};
        Actor water{};
        water.id = ACTOR_OBJ_AQUA;
        patch.unk_1FE = 4; // already planted
        patch.collider.base = { rain ? 0 : AC_HIT, rain ? nullptr : &water };
        beanFlags[3] = false;
        beanRain = rain;
        func_80937FC8(&patch, &field);
        assert(beanFlags[3] && !beanOffered);
        beanPlayerOnTop = true;
        beanPlatformMoved = false;
        func_80938834(&patch, &field);
        assert(beanPlatformMoved);
        ++plantUses;
    }
    std::cout << hookCases << " ability hook decisions, " << ranchCases << " Romani returns with and without the "
              << "ability, " << raceCases << " Gorman fee cases, 2 planting cases and " << plantUses << " planted-bean uses passed\n";
}

inline void TestLegacy10() {
    ApSlot::SlotData slot;
    assert(slot.Parse(json::parse(legacy10Slot)).empty());
    assert(slot.apworldVersion == "0.10.0" && slot.entrancePairs.size() == 49 && slot.mixedEntrancePairs.size() == 70);
    assert(!slot.options.contains("RO_SHUFFLE_EPONA_RIDING") && !slot.options.contains("RO_SHUFFLE_BEAN_PLANTING"));
    LoadSlot(slot);
    const auto& saved = gSaveContext.save.shipSaveInfo.rando;
    legacy10::RandoSaveInfo old{};
    for (auto [name, id] : legacy09::optionIds) {
        if (slot.options.contains(name)) {
            old.randoSaveOptions[id] = slot.options.at(name);
        }
    }
    old.randoSaveOptions[RO_MIX_ENTRANCE_POOLS] = slot.options.at("RO_MIX_ENTRANCE_POOLS");
    old.randoInf[0] = 0x42;
    std::copy(std::begin(saved.entrancePairs), std::end(saved.entrancePairs), old.entrancePairs);
    std::copy(std::begin(saved.mixedEntrancePairs), std::end(saved.mixedEntrancePairs), old.mixedEntrancePairs);
    json encoded;
    legacy10::to_json(encoded, old);
    RandoSaveInfo restored{};
    restored.randoSaveOptions[RO_SHUFFLE_EPONA_RIDING] = 1;
    restored.randoSaveOptions[RO_SHUFFLE_BEAN_PLANTING] = 1;
    std::fill(std::begin(restored.randoInf), std::end(restored.randoInf), 0xFFFF);
    from_json(json::parse(encoded.dump()), restored);
    assert(std::equal(std::begin(old.entrancePairs), std::end(old.entrancePairs), restored.entrancePairs));
    assert(std::equal(std::begin(old.mixedEntrancePairs), std::end(old.mixedEntrancePairs), restored.mixedEntrancePairs));
    assert(std::equal(std::begin(old.randoSaveOptions), std::end(old.randoSaveOptions), restored.randoSaveOptions));
    assert(std::equal(std::begin(old.randoInf), std::end(old.randoInf), restored.randoInf));
    for (const auto& ability : Rando::Abilities::abilities) {
        assert(!restored.randoSaveOptions[ability.option]);
        assert((restored.randoInf[ability.flag / 16] & (1 << (ability.flag % 16))) == 0);
    }
    std::cout << "Real 0.10 slot and historical save codec: 49 original and 70 mixed pairs preserved, abilities off\n";
}

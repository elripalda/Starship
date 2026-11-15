#include <libultraship.h>

#include <Fast3D/interpreter.h>
#include "Engine.h"
#include "extractor/GameExtractor.h"
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include <sf64mesg.h>
    void Main_SetVIMode(void);
    void Main_Initialize(void);
    void Main_ThreadEntry(void* arg);
    void Lib_FillScreen(u8 setFill);
    void Graphics_ThreadUpdate();
    void AudioThread_CreateTask();
}

extern "C"
void Graphics_PushFrame(Gfx* data) {
    GameEngine::ProcessGfxCommands(data);
}

extern "C" void Timer_Update();

void push_frame() {
    Graphics_ThreadUpdate();
    GameEngine::StartAudioFrame();
    GameEngine::Instance->StartFrame();
    Timer_Update();
    // thread5_iteration();
    GameEngine::EndAudioFrame();
}

extern std::string gGameArgPath;

// --prepare: make sf64.o2r from the ROM given with --game, without opening a window.
// Same steps as GameEngine::GenAssetFile: read ROM, check hash, GenerateOTR.
static int Starship_Prepare() {
    if (gGameArgPath.empty()) {
        fprintf(stderr, "--prepare needs --game <path to your Star Fox 64 US .z64 ROM>\n");
        return 1;
    }
    printf("progress 0.0 Reading your ROM\n");
    fflush(stdout);

    // Same context the engine creates first, so the output goes to the same folder.
    auto context = Ship::Context::CreateUninitializedInstance("Starship", "ship", "starship.cfg.json");

    GameExtractor extractor;
    if (!extractor.SelectGameFromUI()) {
        fprintf(stderr, "Could not read the ROM at the path given with --game.\n");
        return 1;
    }

    printf("progress 0.1 Checking your ROM\n");
    fflush(stdout);
    auto game = extractor.ValidateChecksum();
    if (!game.has_value()) {
        fprintf(stderr, "This ROM is not supported. Use a US 1.0 or US 1.1 Star Fox 64 ROM in .z64 format.\n");
        return 1;
    }

    printf("progress 0.2 Extracting assets from %s\n", game.value().c_str());
    fflush(stdout);
    if (!extractor.GenerateOTR()) {
        fprintf(stderr, "Asset extraction failed, no O2R file was generated.\n");
        return 1;
    }

    printf("progress 1.0 Done\n");
    fflush(stdout);
    return 0;
}

#ifdef _WIN32
int SDL_main(int argc, char **argv) {
#else
#if defined(__cplusplus) && defined(PLATFORM_IOS)
extern "C"
#endif
int main(int argc, char *argv[]) {
#endif
    bool prepareOnly = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            gGameArgPath = argv[++i];
        } else if (strcmp(argv[i], "--prepare") == 0) {
            prepareOnly = true;
        }
    }
    if (prepareOnly) {
        return Starship_Prepare();
    }
    GameEngine::Create();
    Main_SetVIMode();
    Lib_FillScreen(1);
    Main_Initialize();
    Main_ThreadEntry(NULL);
    while (WindowIsRunning()) {
        push_frame();
    }
    GameEngine::Instance->Destroy();
    return 0;
}
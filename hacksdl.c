#include <errno.h>
#include <dlfcn.h>
#include <SDL2/SDL.h>
#include "configuration.h"
#include "debug.h"
#include <SDL2/SDL_hidapi.h>
#include <SDL2/SDL_events.h>

// #include "../SDL_internal.h"

// /* Functions for audio drivers to perform runtime conversion of audio format */

// #include "SDL.h"
// #include "SDL_audio.h"
// #include "SDL_audio_c.h"

// #include "SDL_loadso.h"
// #include "../SDL_dataqueue.h"
// #include "SDL_cpuinfo.h"

#define RESAMPLER_BITS_PER_ZERO_CROSSING    3
#define RESAMPLER_SAMPLES_PER_ZERO_CROSSING (1 << RESAMPLER_BITS_PER_ZERO_CROSSING)
#define RESAMPLER_FILTER_INTERP_BITS        (32 - RESAMPLER_BITS_PER_ZERO_CROSSING)
#define RESAMPLER_FILTER_INTERP_RANGE       (1 << RESAMPLER_FILTER_INTERP_BITS)

int initialized = 0;

/*
    Original SDL2 handler
*/
void *sdl_handler = RTLD_NEXT;

extern hacksdl_config_t config;

static Sint32 ResamplerPadding(const Sint32 inrate, const Sint32 outrate);

/*
    Original hooked function
*/
int (*original_SDL_Init)(Uint32 flags);
// int (*original_SDL_Delay)(Uint32 ms);
Uint64 (*original_SDL_GetPerformanceCounter)(void);
Uint64 (*original_SDL_GetPerformanceFrequency)(void);
// void (*original_SDL_WaitThread)(SDL_Thread* thread, int *status);
// int (*original_SDL_SetThreadPriority)(SDL_ThreadPriority priority);

// int (*original_SDL_hid_read_timeout)(SDL_hid_device *dev, unsigned char *data, size_t length, int milliseconds);
// int (*original_SDL_CondWaitTimeout)(SDL_cond * cond, SDL_mutex * mutex, Uint32 ms);
// int (*original_SDL_SemWaitTimeout)(SDL_sem *sem, Uint32 timeout);
// int (*original_SDL_WaitEventTimeout)(SDL_Event * event, int timeout);
// int (*original_SDL_ResampleAudio)(const int chans, const int inrate, 
//     const int outrate,
//     const float *lpadding, const float *rpadding,
//     const float *inbuf, const int inbuflen,
//     float *outbuf, const int outbuflen);


int (*original_SDL_NumJoysticks)(void);

// index related functions
int (*original_SDL_JoystickGetDevicePlayerIndex)(int device_index);
SDL_JoystickGUID (*original_SDL_JoystickGetDeviceGUID)(int device_index);
SDL_Joystick* (*original_SDL_JoystickOpen)(int device_index);
const char* (*original_SDL_JoystickNameForIndex)(int device_index);
Uint16 (*original_SDL_JoystickGetDeviceVendor)(int device_index);
Uint16 (*original_SDL_JoystickGetDeviceProduct)(int device_index);
Uint16 (*original_SDL_JoystickGetDeviceProductVersion)(int device_index);
SDL_JoystickType (*original_SDL_JoystickGetDeviceType)(int device_index);
SDL_JoystickID (*original_SDL_JoystickGetDeviceInstanceID)(int device_index);
SDL_GameController* (*original_SDL_GameControllerOpen)(int joystick_index);
char* (*original_SDL_GameControllerMappingForIndex)(int mapping_index);
SDL_bool (*original_SDL_IsGameController)(int joystick_index);
const char* (*original_SDL_GameControllerNameForIndex)(int joystick_index);
char* (*original_SDL_GameControllerMappingForDeviceIndex)(int joystick_index);

// Value reading functions
Sint16 (*original_SDL_GameControllerGetAxis)(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);
Uint8 (*original_SDL_GameControllerGetButton)(SDL_GameController *gamecontroller, SDL_GameControllerButton button);
SDL_bool (*original_SDL_GameControllerHasAxis)(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);

// Display functions
int (*original_SDL_GetCurrentDisplayMode)(int displayIndex, SDL_DisplayMode * mode);
int (*original_SDL_SetWindowDisplayMode)(SDL_Window * window, const SDL_DisplayMode * mode);
void (*original_SDL_GetWindowSize)(SDL_Window * window, int *w, int *h);
void (*original_SDL_SetWindowSize)(SDL_Window * window, int w, int h);
void (*original_SDL_SetWindowPosition)(SDL_Window * window, int x, int y);
SDL_DisplayMode* (*original_SDL_GetClosestDisplayMode)(int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest);

/*
    Dynamic Library open for original SDL functions
*/
int setup_original_SDL_functions(){

    if(strcmp(config.libsdl_name,"RTLD_NEXT") != 0)
    {
        sdl_handler = dlopen(config.libsdl_name, RTLD_LAZY);
        if (!sdl_handler) {
            HACKSDL_error("Error loading SDL2 library (%s): %s", config.libsdl_name, dlerror());
            return 0;
        }
    }

    original_SDL_Init = dlsym(sdl_handler, "SDL_Init");
    // original_SDL_Delay = dlsym(sdl_handler, "SDL_Delay");
    original_SDL_GetPerformanceCounter = dlsym(sdl_handler, "SDL_GetPerformanceCounter");
    original_SDL_GetPerformanceFrequency = dlsym(sdl_handler, "SDL_GetPerformanceFrequency");
    // original_SDL_hid_read_timeout = dlsym(sdl_handler, "SDL_hid_read_timeout");
    // original_SDL_CondWaitTimeout = dlsym(sdl_handler, "SDL_CondWaitTimeout");
    // original_SDL_SemWaitTimeout = dlsym(sdl_handler, "SDL_SemWaitTimeout");
    // original_SDL_WaitEventTimeout = dlsym(sdl_handler, "SDL_WaitEventTimeout");
    // original_SDL_ResampleAudio = dlsym(sdl_handler, "SDL_ResampleAudio");

    original_SDL_NumJoysticks = dlsym(sdl_handler, "SDL_NumJoysticks");
    original_SDL_JoystickGetDevicePlayerIndex = dlsym(sdl_handler, "SDL_JoystickGetDevicePlayerIndex");
    original_SDL_JoystickGetDeviceGUID = dlsym(sdl_handler, "SDL_JoystickGetDeviceGUID");
    original_SDL_JoystickOpen = dlsym(sdl_handler, "SDL_JoystickOpen");
    original_SDL_JoystickNameForIndex = dlsym(sdl_handler, "SDL_JoystickNameForIndex");
    original_SDL_JoystickGetDeviceVendor = dlsym(sdl_handler, "SDL_JoystickGetDeviceVendor");
    original_SDL_JoystickGetDeviceProduct = dlsym(sdl_handler, "SDL_JoystickGetDeviceProduct");
    original_SDL_JoystickGetDeviceProductVersion = dlsym(sdl_handler, "SDL_JoystickGetDeviceProductVersion");
    original_SDL_JoystickGetDeviceType = dlsym(sdl_handler, "SDL_JoystickGetDeviceType");
    original_SDL_JoystickGetDeviceInstanceID = dlsym(sdl_handler, "SDL_JoystickGetDeviceInstanceID");
    original_SDL_GameControllerOpen = dlsym(sdl_handler, "SDL_GameControllerOpen");
    original_SDL_GameControllerMappingForIndex = dlsym(sdl_handler, "SDL_GameControllerMappingForIndex");
    original_SDL_IsGameController = dlsym(sdl_handler, "SDL_IsGameController");
    original_SDL_GameControllerNameForIndex = dlsym(sdl_handler, "SDL_GameControllerNameForIndex");
    original_SDL_GameControllerMappingForDeviceIndex = dlsym(sdl_handler, "SDL_GameControllerMappingForDeviceIndex");

    original_SDL_GameControllerGetAxis = dlsym(sdl_handler, "SDL_GameControllerGetAxis");
    original_SDL_GameControllerGetButton = dlsym(sdl_handler, "SDL_GameControllerGetButton");
    original_SDL_GameControllerHasAxis = dlsym(sdl_handler, "SDL_GameControllerHasAxis");

    original_SDL_GetCurrentDisplayMode = dlsym(sdl_handler, "SDL_GetCurrentDisplayMode");
    original_SDL_SetWindowDisplayMode = dlsym(sdl_handler, "SDL_SetWindowDisplayMode");
    original_SDL_GetWindowSize = dlsym(sdl_handler, "SDL_GetWindowSize");
    original_SDL_SetWindowSize = dlsym(sdl_handler, "SDL_SetWindowSize");
    original_SDL_SetWindowPosition = dlsym(sdl_handler, "SDL_SetWindowPosition");
    original_SDL_GetClosestDisplayMode = dlsym(sdl_handler, "SDL_GetClosestDisplayMode");
}

/*
    initialize: setup the hack
*/
int initialize()
{
    if(initialized)
    {
        return 1;
    }

    load_config();

    if(setup_original_SDL_functions() == 0)
    {
        HACKSDL_error("Cannot setup SDL hooks");
        return 0;
    }

    HACKSDL_info("Initialization done");

    initialized = 1;
}

/*
    HACKSDL_map_index: return the hacked index for a device
*/
int HACKSDL_map_index(int index)
{
    return config.controller_index_mapping[index];
}

/*
    Functions hooks
*/

int SDL_Init(Uint32 flags)
{
    initialize();

    HACKSDL_info("Hook: flags = %d", flags);
    HACKSDL_info("--");
    // flags = SDL_INIT_AUDIO && SDL_INIT_VIDEO;
    // HACKSDL_info("Hook: new flags-- = %d", flags);
    int ret = original_SDL_Init(flags);
    // HACKSDL_info(SDL_GetCurrentAudioDriver());
    return ret;
}

// void SDL_Delay(Uint32 ms) {
//     HACKSDL_info("SDL_Delay");
// }

// int SDL_hid_read_timeout(SDL_hid_device *dev, unsigned char *data, size_t length, int milliseconds) {
//     HACKSDL_info("SDL_hid_read_timeout");
//     return original_SDL_hid_read_timeout(dev, data, length, 10);
// }

// int SDL_CondWaitTimeout(SDL_cond * cond, SDL_mutex * mutex, Uint32 ms) {
//     HACKSDL_info("SDL_CondWaitTimeout");
//     return original_SDL_CondWaitTimeout(cond, mutex, 10);
// }

// int SDL_SemWaitTimeout(SDL_sem *sem, Uint32 timeout) {
//     HACKSDL_info("SDL_SemWaitTimeout");
//     return original_SDL_SemWaitTimeout(sem, 10);
// }

Uint64 SDL_GetPerformanceCounter(void) {
    Uint64 n = original_SDL_GetPerformanceCounter();
    HACKSDL_info("SDL_GetPerformanceCounter: %lld", n);
    if (n>1000000000000000)
      n = n/8300; 
    return n;
}

Uint64 SDL_GetPerformanceFrequency(void) {
    Uint64 n = original_SDL_GetPerformanceFrequency();
    HACKSDL_info("SDL_GetPerformanceFrequency: %lld", n);
    return n;
}

// void SDL_WaitThread(SDL_Thread * thread, int *status) {
//     HACKSDL_info("SDL_WaitThread");
//     return original_SDL_WaitThread(thread, status);
// }

// int SDL_SetThreadPriority(SDL_ThreadPriority priority) {
//     HACKSDL_info("SDL_SetThreadPriority");
//     return original_SDL_SetThreadPriority(priority);   
// }


// int SDL_WaitEventTimeout(SDL_Event * event, int timeout) {
//     HACKSDL_info("SDL_WaitEventTimeout");
//     return original_SDL_WaitEventTimeout(event, 10);
// }

// SDL_AudioDeviceID SDL_OpenAudioDevice(
//     const char *device,
//     int iscapture,
//     const SDL_AudioSpec *desired,
//     SDL_AudioSpec *obtained,
//     int allowed_changes) {
//         HACKSDL_info("SDL_OpenAudioDevice");

//         return 0;
// }

// int SDL_OpenAudio(SDL_AudioSpec * desired,
//     SDL_AudioSpec * obtained) {
//         HACKSDL_info("SDL_OpenAudio");
//         return 0;
// }

// static Sint32
// ResamplerPadding(const Sint32 inrate, const Sint32 outrate)
// {
//     /* This function uses integer arithmetics to avoid precision loss caused
//      * by large floating point numbers. Sint32 is needed for the large number
//      * multiplication. The integers are assumed to be non-negative so that
//      * division rounds by truncation. */
//     if (inrate == outrate) {
//         return 0;
//     }
//     if (inrate > outrate) {
//         return (RESAMPLER_SAMPLES_PER_ZERO_CROSSING * inrate + outrate - 1) / outrate;
//     }
//     return RESAMPLER_SAMPLES_PER_ZERO_CROSSING;
// }

// static int
// SDL_ResampleAudio(const int chans, const int inrate, const int outrate,
//                         const float *lpadding, const float *rpadding,
//                         const float *inbuf, const int inbuflen,
//                         float *outbuf, const int outbuflen)
// {
//     HACKSDL_info("SDL_ResampleAudio");
//     /* This function uses integer arithmetics to avoid precision loss caused
//      * by large floating point numbers. For some operations, Sint32 or Sint64
//      * are needed for the large number multiplications. The input integers are
//      * assumed to be non-negative so that division rounds by truncation and
//      * modulo is always non-negative. Note that the operator order is important
//      * for these integer divisions. */
//     const int paddinglen = ResamplerPadding(inrate, outrate);
//     const int framelen = chans * (int)sizeof (float);
//     const int inframes = inbuflen / framelen;
//     /* outbuflen isn't total to write, it's total available. */
//     const int wantedoutframes = ((Sint64) inframes) * outrate / inrate;
//     const int maxoutframes = outbuflen / framelen;
//     const int outframes = SDL_min(wantedoutframes, maxoutframes);
//     // float *dst = outbuf;
//     // int i, j, chan;

//     // for (i = 0; i < outframes; i++) {
//     //     const int srcindex = ((Sint64) i) * inrate / outrate;
//     //     /* Calculating the following way avoids subtraction or modulo of large
//     //      * floats which have low result precision.
//     //      *   interpolation1
//     //      * = (i / outrate * inrate) - floor(i / outrate * inrate)
//     //      * = mod(i / outrate * inrate, 1)
//     //      * = mod(i * inrate, outrate) / outrate */
//     //     const int srcfraction = ((Sint64) i) * inrate % outrate;
//     //     const float interpolation1 = ((float) srcfraction) / ((float) outrate);
//     //     const int filterindex1 = ((Sint32) srcfraction) * RESAMPLER_SAMPLES_PER_ZERO_CROSSING / outrate;
//     //     const float interpolation2 = 1.0f - interpolation1;
//     //     const int filterindex2 = ((Sint32) (outrate - srcfraction)) * RESAMPLER_SAMPLES_PER_ZERO_CROSSING / outrate;

//     //     for (chan = 0; chan < chans; chan++) {
//     //         float outsample = 0.0f;

//     //         /* do this twice to calculate the sample, once for the "left wing" and then same for the right. */
//     //         for (j = 0; (filterindex1 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)) < RESAMPLER_FILTER_SIZE; j++) {
//     //             const int filt_ind = filterindex1 + j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING;
//     //             const int srcframe = srcindex - j;
//     //             /* !!! FIXME: we can bubble this conditional out of here by doing a pre loop. */
//     //             const float insample = (srcframe < 0) ? lpadding[((paddinglen + srcframe) * chans) + chan] : inbuf[(srcframe * chans) + chan];
//     //             outsample += (float)(insample * (ResamplerFilter[filt_ind] + (interpolation1 * ResamplerFilterDifference[filt_ind])));
//     //         }

//     //         /* Do the right wing! */
//     //         for (j = 0; (filterindex2 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)) < RESAMPLER_FILTER_SIZE; j++) {
//     //             const int filt_ind = filterindex2 + j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING;
//     //             const int srcframe = srcindex + 1 + j;
//     //             /* !!! FIXME: we can bubble this conditional out of here by doing a post loop. */
//     //             const float insample = (srcframe >= inframes) ? rpadding[((srcframe - inframes) * chans) + chan] : inbuf[(srcframe * chans) + chan];
//     //             outsample += (float)(insample * (ResamplerFilter[filt_ind] + (interpolation2 * ResamplerFilterDifference[filt_ind])));
//     //         }

//     //         *(dst++) = outsample;
//     //     }
//     // }

//     return outframes * chans * sizeof (float);
// }

int SDL_NumJoysticks(void)
{
    if(config.no_gamecontroller)
    {       
        HACKSDL_debug("Hook + hack: return 0");
        return 0;
    }
    else
    {
        int result = original_SDL_NumJoysticks();
        HACKSDL_debug("Hook: return = %d", result);
        return result;
    }
}

int SDL_JoystickGetDevicePlayerIndex(int device_index)
{
    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDevicePlayerIndex(HACKSDL_map_index(device_index));
}

SDL_JoystickGUID SDL_JoystickGetDeviceGUID(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceGUID(HACKSDL_map_index(device_index));

}

SDL_Joystick* SDL_JoystickOpen(int device_index)
{

    if(config.no_gamecontroller == 2 || config.device_disable[device_index] == 2)
    {
        HACKSDL_debug("Hook + hack: return NULL for device_index=%d", device_index);
        return NULL;
    }

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }
    
    return original_SDL_JoystickOpen(HACKSDL_map_index(device_index));
}

const char* SDL_JoystickNameForIndex(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickNameForIndex(HACKSDL_map_index(device_index));
}

Uint16 SDL_JoystickGetDeviceVendor(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceVendor(HACKSDL_map_index(device_index));
}

Uint16 SDL_JoystickGetDeviceProduct(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceProduct(HACKSDL_map_index(device_index));
}

Uint16 SDL_JoystickGetDeviceProductVersion(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceProductVersion(HACKSDL_map_index(device_index));
}

SDL_JoystickType SDL_JoystickGetDeviceType(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceType(HACKSDL_map_index(device_index));
}

SDL_JoystickID SDL_JoystickGetDeviceInstanceID(int device_index)
{

    if(HACKSDL_map_index(device_index) != device_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", device_index, HACKSDL_map_index(device_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", device_index);
    }

    return original_SDL_JoystickGetDeviceInstanceID(HACKSDL_map_index(device_index));
}

SDL_GameController* SDL_GameControllerOpen(int joystick_index)
{    
    if(config.no_gamecontroller == 2 || config.device_disable[joystick_index] == 2)
    {
        HACKSDL_debug("Hook + hack: return NULL for joystick_index=%d", joystick_index);
        return NULL;
    }

    if(HACKSDL_map_index(joystick_index) != joystick_index){
        HACKSDL_debug("Hook + hack: joystick_index %d -> %d)", joystick_index, HACKSDL_map_index(joystick_index));
    }
    else
    {
        HACKSDL_debug("Hook: device_index=%d", joystick_index);
    }

    return original_SDL_GameControllerOpen(HACKSDL_map_index(joystick_index));;
}

char* SDL_GameControllerMappingForIndex(int mapping_index)
{

    if(HACKSDL_map_index(mapping_index) != mapping_index){
        HACKSDL_debug("Hook + hack: device_index %d -> %d)", mapping_index, HACKSDL_map_index(mapping_index));
    }
    else
    {
        HACKSDL_debug("Hook: mapping_index=%d", mapping_index);
    }
    
    return original_SDL_GameControllerMappingForIndex(HACKSDL_map_index(mapping_index));
}

SDL_bool SDL_IsGameController(int joystick_index)
{

    if(config.no_gamecontroller || config.device_disable[joystick_index]){
        HACKSDL_debug("Hook + hack: return false for joystick_index=%d", joystick_index);
        return SDL_FALSE;
    }else{
        if(HACKSDL_map_index(joystick_index) != joystick_index){
            HACKSDL_debug("Hook + hack: joystick_index %d -> %d)", joystick_index, HACKSDL_map_index(joystick_index));
        }
        else
        {
            HACKSDL_debug("Hook: joystick_index=%d", joystick_index);
        }
        return original_SDL_IsGameController(HACKSDL_map_index(joystick_index));
    }
}

const char* SDL_GameControllerNameForIndex(int joystick_index)
{

    if(HACKSDL_map_index(joystick_index) != joystick_index){
        HACKSDL_debug("Hook + hack: joystick_index %d -> %d)", joystick_index, HACKSDL_map_index(joystick_index));
    }
    else
    {
        HACKSDL_debug("Hook: joystick_index=%d", joystick_index);
    }
    
    return original_SDL_GameControllerNameForIndex(HACKSDL_map_index(joystick_index));
}

char* SDL_GameControllerMappingForDeviceIndex(int joystick_index)
{

    if(HACKSDL_map_index(joystick_index) != joystick_index){
        HACKSDL_debug("Hook + hack: joystick_index %d -> %d)", joystick_index, HACKSDL_map_index(joystick_index));
    }
    else
    {
        HACKSDL_debug("Hook: joystick_index=%d", joystick_index);
    }
    
    return original_SDL_GameControllerMappingForDeviceIndex(HACKSDL_map_index(joystick_index));
}

void SDL_GameControllerUpdate(void)
{
    void (*original_SDL_GameControllerUpdate)(void);
    original_SDL_GameControllerUpdate = dlsym(RTLD_NEXT, "SDL_GameControllerUpdate");

    HACKSDL_debug("hook on SDL_GameControllerUpdate()");

    original_SDL_GameControllerUpdate();
}

Sint16 SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis)
{

    Sint16 axis_value = 0;
    Uint8 button_pressed = 0;
    int axis_minus_virtual_button_value = 0;
    int axis_plus_virtual_button_value = 0;

    axis_value = original_SDL_GameControllerGetAxis(gamecontroller, axis);

    HACKSDL_debug("Hook gamecontroller=%s axis=%s value=%d", SDL_GameControllerName(gamecontroller), SDL_GameControllerGetStringForAxis(axis), axis_value);

    if ( (config.axis_virtual_minus_map[axis] != SDL_CONTROLLER_BUTTON_INVALID) || (config.axis_virtual_plus_map[axis] != SDL_CONTROLLER_BUTTON_INVALID) )
    {
        if ( (original_SDL_GameControllerGetButton(gamecontroller,config.axis_virtual_hotkey[axis])) || (config.axis_virtual_hotkey[axis] == SDL_CONTROLLER_BUTTON_INVALID))
        {
            axis_minus_virtual_button_value = original_SDL_GameControllerGetButton(gamecontroller, config.axis_virtual_minus_map[axis]);
            axis_plus_virtual_button_value = original_SDL_GameControllerGetButton(gamecontroller, config.axis_virtual_plus_map[axis]);
            
            if(axis_minus_virtual_button_value && (! axis_plus_virtual_button_value))
            {
                axis_value = config.axis_virtual_min[axis];
            }
            else if((! axis_minus_virtual_button_value) && axis_plus_virtual_button_value)
            {
                axis_value = config.axis_virtual_max[axis];
            }
            else if(config.axis_virtual_merge[axis] == 0)
            {
                axis_value = 0;
            }

            HACKSDL_debug("Virtual axis enabled, new value=%d", axis_value);
        }
    }

    if(config.axis_modifier_shift[axis] != 0)
    {
        button_pressed = original_SDL_GameControllerGetButton(gamecontroller, config.modifier_button);

        if (button_pressed == 1)
        {
            axis_value = axis_value >> config.axis_modifier_shift[axis];
            HACKSDL_debug("Modifier enabled, new value=%d", axis_value);
        }
    }
    
    if(config.axis_deadzone[axis] > 0)
    {
        if(axis_value < config.axis_deadzone[axis])
        {
            // deadzone
            axis_value = 0;
            HACKSDL_debug("Deadzone enabled, new value=%d", axis_value);
        }
        else if(config.axis_digital[axis] = 1)
        {
            // digital mode
            if(axis_value < 0)
            {
                axis_value = SDL_AXIS_MIN;
            }
            else if(axis_value > 0)
            {
                axis_value = SDL_AXIS_MAX;
            }
            HACKSDL_debug("Digital mode, new value=%d", axis_value);
        }

    }

    return axis_value;

}

Uint8 SDL_GameControllerGetButton(SDL_GameController *gamecontroller, SDL_GameControllerButton button)
{
    Uint8 button_pressed = 0;

    button_pressed = original_SDL_GameControllerGetButton(gamecontroller, button);

    HACKSDL_debug("Hook gamecontroller=%s button=%s value=%d", SDL_GameControllerName(gamecontroller), SDL_GameControllerGetStringForButton(button), button_pressed);

    if(config.button_disable[button])
    {
        HACKSDL_debug("Button disabled, new value=0");
        return 0;
    }else if (original_SDL_GameControllerGetButton(gamecontroller,config.button_disable_key[button]) && (config.button_disable_key[button] != SDL_CONTROLLER_BUTTON_INVALID))
    {
        return 0;
    }

    return button_pressed;

}

SDL_bool SDL_GameControllerHasAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis)
{
    SDL_bool has_axis = SDL_FALSE;

    has_axis = SDL_GameControllerHasAxis(gamecontroller, axis);

    HACKSDL_debug("Hook gamecontroller=%s button=%s value=%d", SDL_GameControllerName(gamecontroller), SDL_GameControllerGetStringForAxis(axis), has_axis);

    // Return true if this axis is virtual
    if ((config.axis_virtual_minus_map[axis] != SDL_CONTROLLER_BUTTON_INVALID) || (config.axis_virtual_plus_map[axis] != SDL_CONTROLLER_BUTTON_INVALID))
    {
        HACKSDL_debug("Virtual axis enabled, new value=1");
    }

    return has_axis;

}

int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode * mode)
{
    int result = 0;
    result = original_SDL_GetCurrentDisplayMode(displayIndex, mode);
    HACKSDL_debug("result=%d", result);
    if(result == 0)
    {
        HACKSDL_debug("mode.w=%d mode.h=%d", mode->w,mode->h);
    }

    if (config.get_display_mode_w != 0)
    {
        mode->w = config.get_display_mode_w;
        HACKSDL_debug("new value mode.w=%d", mode->w);
    }

    if (config.get_display_mode_h != 0)
    {
        mode->h = config.get_display_mode_h;
        HACKSDL_debug("new value mode.h=%d", mode->h);
    }

    return result;

}

int SDL_SetWindowDisplayMode(SDL_Window * window, const SDL_DisplayMode * mode)
{
    HACKSDL_debug("mode.w=%d mode.h=%d", mode->w, mode->h);

    SDL_DisplayMode * new_mode;
    memcpy((void*)mode, (void*)new_mode, sizeof(SDL_DisplayMode)); // UNTESTED !


    if (config.set_display_mode_w != 0)
    {
        new_mode->w = config.set_display_mode_w;
        HACKSDL_debug("new value mode.w=%d", new_mode->w);
    }

    if (config.set_display_mode_h != 0)
    {
        new_mode->h = config.set_display_mode_h;
        HACKSDL_debug("new value mode.h=%d", new_mode->h);
    }
    
    original_SDL_SetWindowDisplayMode(window, new_mode);
}

void SDL_GetWindowSize(SDL_Window * window, int *w, int *h)
{
    original_SDL_GetWindowSize(window, w, h);
    HACKSDL_debug("w=%d h=%d", *w, *h);
    if (config.get_window_size_w != 0)
    {
        *w = config.get_window_size_w;
        HACKSDL_debug("new value w=%d" , *w);
    }
    if (config.get_window_size_h != 0)
    {
        *h = config.get_window_size_h;
        HACKSDL_debug("new value w=%d" , *h);
    }
}

void SDL_SetWindowSize(SDL_Window * window, int w, int h)
{
    HACKSDL_debug("w=%d h=%d", w, h);

    if (config.set_window_size_w != 0)
    {
        w = config.set_window_size_w;
        HACKSDL_debug("new value w=%d", w);
    }

    if (config.set_window_size_h != 0)
    {
        h = config.set_window_size_h;
        HACKSDL_debug("new value h=%d", h);
    }

    original_SDL_SetWindowSize(window, w, h);
}

SDL_DisplayMode * SDL_GetClosestDisplayMode(int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest)
{
    SDL_DisplayMode * result;
    result = original_SDL_GetClosestDisplayMode(displayIndex, mode, closest);

        HACKSDL_debug("mode.w=%d mode.h=%d closest.w=%d closest.h=%d", mode->w, mode->h, closest->w, closest->h);
        if (result != NULL)
        {
            HACKSDL_debug("result.w=%d result.h=%d closest.w=%d closest.h=%d", result->w, result->h);
        }

    return result;
}

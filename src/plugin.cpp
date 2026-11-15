// Simple White Noise: a CLAP plugin that generates stereo white noise with one volume parameter.
// SPDX-License-Identifier: MIT

#include <clap/clap.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {

constexpr clap_id kVolumeParamId = 0;
constexpr double kVolumeMinDb = -60.0;
constexpr double kVolumeMaxDb = 0.0;
constexpr double kVolumeDefaultDb = -18.0;

// The state is a fixed header followed by the volume as a little-endian IEEE 754 double.
constexpr char kStateMagic[4] = {'S', 'W', 'N', '1'};
constexpr size_t kStateSize = sizeof(kStateMagic) + sizeof(double);

const char *const kFeatures[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, CLAP_PLUGIN_FEATURE_STEREO, nullptr};

const clap_plugin_descriptor_t kDescriptor = {
    CLAP_VERSION_INIT,
    "jp.allegromoltov.simple-whitenoise",
    "Simple White Noise",
    "AllegroMoltoV",
    "https://github.com/AllegroMoltoV/simple-whitenoise-audio",
    "https://github.com/AllegroMoltoV/simple-whitenoise-audio#readme",
    "https://github.com/AllegroMoltoV/simple-whitenoise-audio/issues",
    SWN_VERSION,
    "Generates stereo white noise with a single volume control.",
    kFeatures,
};

double clampDb(double db) {
    if (!std::isfinite(db)) return kVolumeDefaultDb;
    if (db < kVolumeMinDb) return kVolumeMinDb;
    if (db > kVolumeMaxDb) return kVolumeMaxDb;
    return db;
}

float dbToGain(double db) { return static_cast<float>(std::pow(10.0, db / 20.0)); }

struct Plugin {
    clap_plugin_t clap;
    const clap_host_t *host;

    // Written by the audio thread (parameter events) and by the main thread (state load,
    // flush while not processing); read by both. A lock-free atomic keeps the audio thread
    // free of locks.
    std::atomic<double> volumeDb{kVolumeDefaultDb};

    // Audio thread only.
    float currentGain = 0.0f;
    float smoothing = 0.0f;
    uint32_t rng[2] = {0x9E3779B9u, 0x7F4A7C15u};
};

static_assert(std::atomic<double>::is_always_lock_free, "atomic<double> must be lock-free");

Plugin *self(const clap_plugin_t *p) { return static_cast<Plugin *>(p->plugin_data); }

// xorshift32: uniform values in [-1, 1).
float nextNoise(uint32_t &s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return static_cast<float>(static_cast<int32_t>(s)) * (1.0f / 2147483648.0f);
}

void applyEvent(Plugin *plug, const clap_event_header_t *hdr) {
    if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID || hdr->type != CLAP_EVENT_PARAM_VALUE) return;
    const auto *ev = reinterpret_cast<const clap_event_param_value_t *>(hdr);
    if (ev->param_id == kVolumeParamId) plug->volumeDb.store(clampDb(ev->value), std::memory_order_relaxed);
}

// ---- audio ports ----

uint32_t audioPortsCount(const clap_plugin_t *, bool isInput) { return isInput ? 0 : 1; }

bool audioPortsGet(const clap_plugin_t *, uint32_t index, bool isInput, clap_audio_port_info_t *info) {
    if (isInput || index != 0) return false;
    info->id = 0;
    std::snprintf(info->name, sizeof(info->name), "%s", "Output");
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

const clap_plugin_audio_ports_t kAudioPorts = {audioPortsCount, audioPortsGet};

// ---- params ----

uint32_t paramsCount(const clap_plugin_t *) { return 1; }

bool paramsGetInfo(const clap_plugin_t *, uint32_t index, clap_param_info_t *info) {
    if (index != 0) return false;
    std::memset(info, 0, sizeof(*info));
    info->id = kVolumeParamId;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    info->cookie = nullptr;
    std::snprintf(info->name, sizeof(info->name), "%s", "Volume");
    info->module[0] = '\0';
    info->min_value = kVolumeMinDb;
    info->max_value = kVolumeMaxDb;
    info->default_value = kVolumeDefaultDb;
    return true;
}

bool paramsGetValue(const clap_plugin_t *p, clap_id id, double *value) {
    if (id != kVolumeParamId) return false;
    *value = self(p)->volumeDb.load(std::memory_order_relaxed);
    return true;
}

bool paramsValueToText(const clap_plugin_t *, clap_id id, double value, char *display, uint32_t size) {
    if (id != kVolumeParamId || size == 0) return false;
    std::snprintf(display, size, "%.1f dB", value);
    return true;
}

bool paramsTextToValue(const clap_plugin_t *, clap_id id, const char *display, double *value) {
    if (id != kVolumeParamId) return false;
    char *end = nullptr;
    const double v = std::strtod(display, &end);
    if (end == display || !std::isfinite(v)) return false;
    *value = clampDb(v);
    return true;
}

void paramsFlush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *) {
    const uint32_t n = in->size(in);
    for (uint32_t i = 0; i < n; ++i) applyEvent(self(p), in->get(in, i));
}

const clap_plugin_params_t kParams = {paramsCount,       paramsGetInfo,     paramsGetValue,
                                      paramsValueToText, paramsTextToValue, paramsFlush};

// ---- state ----

bool stateSave(const clap_plugin_t *p, const clap_ostream_t *stream) {
    unsigned char buf[kStateSize];
    std::memcpy(buf, kStateMagic, sizeof(kStateMagic));
    const double v = self(p)->volumeDb.load(std::memory_order_relaxed);
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    for (size_t i = 0; i < 8; ++i) buf[sizeof(kStateMagic) + i] = static_cast<unsigned char>(bits >> (8 * i));

    size_t written = 0;
    while (written < kStateSize) {
        const int64_t r = stream->write(stream, buf + written, kStateSize - written);
        if (r <= 0) return false;
        written += static_cast<size_t>(r);
    }
    return true;
}

bool stateLoad(const clap_plugin_t *p, const clap_istream_t *stream) {
    unsigned char buf[kStateSize];
    size_t got = 0;
    while (got < kStateSize) {
        const int64_t r = stream->read(stream, buf + got, kStateSize - got);
        if (r <= 0) return false;
        got += static_cast<size_t>(r);
    }
    if (std::memcmp(buf, kStateMagic, sizeof(kStateMagic)) != 0) return false;
    uint64_t bits = 0;
    for (size_t i = 0; i < 8; ++i) bits |= static_cast<uint64_t>(buf[sizeof(kStateMagic) + i]) << (8 * i);
    double v;
    std::memcpy(&v, &bits, sizeof(v));

    Plugin *plug = self(p);
    plug->volumeDb.store(clampDb(v), std::memory_order_relaxed);
    const auto *hostParams =
        static_cast<const clap_host_params_t *>(plug->host->get_extension(plug->host, CLAP_EXT_PARAMS));
    if (hostParams && hostParams->rescan) hostParams->rescan(plug->host, CLAP_PARAM_RESCAN_VALUES);
    return true;
}

const clap_plugin_state_t kState = {stateSave, stateLoad};

// ---- plugin ----

bool pluginInit(const clap_plugin_t *) { return true; }

void pluginDestroy(const clap_plugin_t *p) { delete self(p); }

bool pluginActivate(const clap_plugin_t *p, double sampleRate, uint32_t, uint32_t) {
    Plugin *plug = self(p);
    // One-pole smoothing with a time constant of about 10 ms avoids clicks on volume changes.
    plug->smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.010 * sampleRate)));
    plug->currentGain = dbToGain(plug->volumeDb.load(std::memory_order_relaxed));
    return true;
}

void pluginDeactivate(const clap_plugin_t *) {}

bool pluginStartProcessing(const clap_plugin_t *) { return true; }

void pluginStopProcessing(const clap_plugin_t *) {}

void pluginReset(const clap_plugin_t *p) {
    Plugin *plug = self(p);
    plug->currentGain = dbToGain(plug->volumeDb.load(std::memory_order_relaxed));
}

clap_process_status pluginProcess(const clap_plugin_t *p, const clap_process_t *process) {
    Plugin *plug = self(p);
    const uint32_t frames = process->frames_count;
    const uint32_t eventCount = process->in_events->size(process->in_events);
    uint32_t eventIndex = 0;

    float *out[2] = {nullptr, nullptr};
    if (process->audio_outputs_count > 0 && process->audio_outputs[0].channel_count >= 2) {
        out[0] = process->audio_outputs[0].data32[0];
        out[1] = process->audio_outputs[0].data32[1];
    }

    float target = dbToGain(plug->volumeDb.load(std::memory_order_relaxed));
    for (uint32_t i = 0; i < frames; ++i) {
        // Apply parameter events at their sample position.
        while (eventIndex < eventCount) {
            const clap_event_header_t *hdr = process->in_events->get(process->in_events, eventIndex);
            if (hdr->time > i) break;
            applyEvent(plug, hdr);
            target = dbToGain(plug->volumeDb.load(std::memory_order_relaxed));
            ++eventIndex;
        }
        plug->currentGain += (target - plug->currentGain) * plug->smoothing;
        if (out[0]) {
            out[0][i] = nextNoise(plug->rng[0]) * plug->currentGain;
            out[1][i] = nextNoise(plug->rng[1]) * plug->currentGain;
        }
    }
    // Events at or after the end of the block.
    for (; eventIndex < eventCount; ++eventIndex) applyEvent(plug, process->in_events->get(process->in_events, eventIndex));

    if (process->audio_outputs_count > 0) process->audio_outputs[0].constant_mask = 0;
    return CLAP_PROCESS_CONTINUE;
}

const void *pluginGetExtension(const clap_plugin_t *, const char *id) {
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &kAudioPorts;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &kParams;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &kState;
    return nullptr;
}

void pluginOnMainThread(const clap_plugin_t *) {}

// ---- factory and entry ----

uint32_t factoryGetPluginCount(const clap_plugin_factory_t *) { return 1; }

const clap_plugin_descriptor_t *factoryGetPluginDescriptor(const clap_plugin_factory_t *, uint32_t index) {
    return index == 0 ? &kDescriptor : nullptr;
}

const clap_plugin_t *factoryCreatePlugin(const clap_plugin_factory_t *, const clap_host_t *host, const char *pluginId) {
    if (!clap_version_is_compatible(host->clap_version) || std::strcmp(pluginId, kDescriptor.id) != 0) return nullptr;
    auto *plug = new (std::nothrow) Plugin();
    if (!plug) return nullptr;
    plug->host = host;
    plug->clap = {&kDescriptor,          plug,
                  pluginInit,            pluginDestroy,
                  pluginActivate,        pluginDeactivate,
                  pluginStartProcessing, pluginStopProcessing,
                  pluginReset,           pluginProcess,
                  pluginGetExtension,    pluginOnMainThread};
    return &plug->clap;
}

const clap_plugin_factory_t kFactory = {factoryGetPluginCount, factoryGetPluginDescriptor, factoryCreatePlugin};

bool entryInit(const char *) { return true; }

void entryDeinit() {}

const void *entryGetFactory(const char *factoryId) {
    return std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0 ? &kFactory : nullptr;
}

} // namespace

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entryInit, entryDeinit, entryGetFactory};

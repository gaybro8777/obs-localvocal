#ifndef TRANSCRIPTION_FILTER_DATA_H
#define TRANSCRIPTION_FILTER_DATA_H

#include <obs.h>
#include <util/circlebuf.h>
#include <util/darray.h>
#include <media-io/audio-resampler.h>

#include <whisper.h>

#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>

#include "translation/translation.h"
#include "whisper-utils/silero-vad-onnx.h"
#include "whisper-utils/whisper-processing.h"
#include "whisper-utils/token-buffer-thread.h"

#define MAX_PREPROC_CHANNELS 10

#define MT_ obs_module_text

struct transcription_filter_data {
	obs_source_t *context; // obs filter source (this filter)
	size_t channels;       // number of channels
	uint32_t sample_rate;  // input sample rate
	// How many input frames (in input sample rate) are needed for the next whisper frame
	size_t frames;
	// How many ms/frames are needed to overlap with the next whisper frame
	size_t overlap_frames;
	size_t overlap_ms;
	// How many frames were processed in the last whisper frame (this is dynamic)
	size_t last_num_frames;
	// Milliseconds per processing step (e.g. rest of the whisper buffer may be filled with silence)
	size_t step_size_msec;
	// Start begining timestamp in ms since epoch
	uint64_t start_timestamp_ms;
	// Sentence counter for srt
	size_t sentence_number;
	// Minimal subtitle duration in ms
	size_t min_sub_duration;
	// Last time a subtitle was rendered
	uint64_t last_sub_render_time;

	/* PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
	struct circlebuf info_buffer;
	struct circlebuf input_buffers[MAX_PREPROC_CHANNELS];

	/* Resampler */
	audio_resampler_t *resampler_to_whisper;

	/* whisper */
	std::string whisper_model_path;
	struct whisper_context *whisper_context;
	whisper_full_params whisper_params;

	/* Silero VAD */
	std::unique_ptr<VadIterator> vad;

	float filler_p_threshold;

	bool do_silence;
	bool vad_enabled;
	int log_level = LOG_DEBUG;
	bool log_words;
	bool caption_to_stream;
	bool active = false;
	bool save_srt = false;
	bool truncate_output_file = false;
	bool save_only_while_recording = false;
	bool process_while_muted = false;
	bool rename_file_to_match_recording = false;
	bool translate = false;
	std::string source_lang;
	std::string target_lang;
	std::string translation_output;
	bool buffered_output = false;
	bool enable_token_ts_dtw = false;
	std::string suppress_sentences;

	// Last transcription result
	std::string last_text;

	// Text source to output the subtitles
	std::string text_source_name;
	// Callback to set the text in the output text source (subtitles)
	std::function<void(const DetectionResultWithText &result)> setTextCallback;
	// Output file path to write the subtitles
	std::string output_file_path;
	std::string whisper_model_file_currently_loaded;

	// Use std for thread and mutex
	std::thread whisper_thread;

	std::mutex *whisper_buf_mutex;
	std::mutex *whisper_ctx_mutex;
	std::condition_variable *wshiper_thread_cv;

	// translation context
	struct translation_context translation_ctx;

	TokenBufferThread captions_monitor;

	// ctor
	transcription_filter_data()
	{
		// initialize all pointers to nullptr
		for (size_t i = 0; i < MAX_PREPROC_CHANNELS; i++) {
			copy_buffers[i] = nullptr;
		}
		context = nullptr;
		resampler_to_whisper = nullptr;
		whisper_model_path = "";
		whisper_context = nullptr;
		whisper_buf_mutex = nullptr;
		whisper_ctx_mutex = nullptr;
		wshiper_thread_cv = nullptr;
		output_file_path = "";
		whisper_model_file_currently_loaded = "";
	}
};

// Audio packet info
struct transcription_filter_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

void set_text_callback(struct transcription_filter_data *gf, const DetectionResultWithText &str);

#endif /* TRANSCRIPTION_FILTER_DATA_H */

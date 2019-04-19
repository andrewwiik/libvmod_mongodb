#define MONGO_INFO_LOG(ctx, fmt, ...) \
	do { \
		const struct vrt_ctx *_ctx = ctx; \
    char *_buffer; \
		assert(asprintf( \
                &_buffer, \
                "[MONGODB] %s", fmt) > 0); \
		VSLb(_ctx->vsl, SLT_VCL_Log, _buffer, ##__VA_ARGS__); \
		free(_buffer); \
	} while (0)
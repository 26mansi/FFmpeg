#include "parser.h"

typedef struct APNGParseContext {
    ParseContext pc;
    uint32_t chunk_pos;           ///< position inside current chunk
    uint32_t chunk_length;        ///< length of the current chunk
    uint32_t remaining_size;      ///< remaining size of the current chunk
} APNGParseContext;

static int apng_parse(AVCodecParserContext *s, AVCodecContext *avctx,
                     const uint8_t **poutbuf, int *poutbuf_size,
                     const uint8_t *buf, int buf_size)
{
    APNGParseContext *apc = s->priv_data;
    int next = END_NOT_FOUND;
    int i = 0;

    s->pict_type = AV_PICTURE_TYPE_NONE;

    *poutbuf_size = 0;

    if (!apc->pc.frame_start_found) {
        uint32_t state = apc->pc.state;
        for (; i < buf_size; i++) {
            state = (state << 8) | buf[i];
            if (state == MKTAG('f','c', 'T','L')  || state == MKTAG('I', 'H', 'D', 'R')){
                i++;
                apc->pc.frame_start_found = 1;
                break;
            }
        }
        apc->pc.state = state;
    } else if (apc->remaining_size) {
        i = FFMIN(apc->remaining_size, buf_size);
        apc->remaining_size -= i;
        if (apc->remaining_size)
            goto flush;
        if (apc->chunk_pos == -1) {
            next = i;
            goto flush;
        }
    }

    for (; apc->pc.frame_start_found && i < buf_size; i++) {
        apc->pc.state = (apc->pc.state << 8) | buf[i];
        if (apc->chunk_pos == 3) {
            apc->chunk_length = apc->pc.state;
            if (apc->chunk_length > 0x7fffffff) {
                apc->chunk_pos = apc->pc.frame_start_found = 0;
                goto flush;
            }
            apc->chunk_length += 4;
        } else if (apc->chunk_pos == 7) {
            if (apc->chunk_length >= buf_size - i)
                apc->remaining_size = apc->chunk_length - buf_size + i + 1;
            
            if (apc->pc.state == MKTAG('f', 'c', 'T', 'L')){
                next = i - 8;
                goto flush;

            }

            if (apc->pc.state == MKTAG('I', 'E', 'N', 'D') ) { 
                if (apc->remaining_size)
                    apc->chunk_pos = -1;
                else
                    next = apc->chunk_length + i + 1;
                break;
            } else {
                apc->chunk_pos = 0;
                if (apc->remaining_size)
                    break;
                else
                    i += apc->chunk_length;
                continue;
            }
        }
        apc->chunk_pos++;
    }

flush:
    if (ff_combine_frame(&apc->pc, next, &buf, &buf_size) < 0)
        return buf_size;

    apc->chunk_pos = apc->pc.frame_start_found = 0;

    *poutbuf      = buf;
    *poutbuf_size = buf_size;
    return next;
}

AVCodecParser ff_apng_parser = {
    .codec_ids      = { AV_CODEC_ID_PNG },
    .priv_data_size = sizeof(APNGParseContext),
    .parser_parse   = apng_parse,
    .parser_close   = ff_parse_close,
};

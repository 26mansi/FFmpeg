#include "avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/bytestream.h"
#include "rawdec.h"
//#include "libavcodec/png.h"
#include "avio.h"

typedef struct APNGDemuxContext {
    const AVClass *class;
    int nb_loops;
    int width, height;
    int bit_depth;
    int color_type;
    int compression_type;
    int interlace_type;
    int filter_type;
    int channels; 




} APNGDemuxContext;



static int apng_probe(AVProbeData *p)
{      
        uint32_t tag, length; 
        GetByteContext gb;
        uint64_t PNGSIG = 0x89504e470d0a1a0a;
        bytestream2_init(&gb,p->buf, p->buf_size);

        if   ( PNGSIG != bytestream2_get_be64(&gb));
            return 0;

        while (bytestream2_get_bytes_left(&gb) > 0 ){
            length = bytestream2_get_be32(&gb);
            tag = bytestream2_get_le32(&gb);
            if (tag == MKTAG('I', 'D', 'A', 'T'))
                return 0;
            if (tag == MKTAG('a', 'c', 'T', 'L'))
                return AVPROBE_SCORE_MAX;
            else  bytestream2_skip(&gb, length + 4);
            }
        return 0;
}

/*
static int raw_read_packet(AVFormatContext *s, AVPacket *pkt)
{       APNGDemuxContext *apdc = s->priv_data;
        int ret;
        return ret;  
 }
*/

static int read_header(AVFormatContext *s)
{       APNGDemuxContext *apdc = s->priv_data;
        int ret = -1;
        AVIOContext *pb     = s->pb;
        AVStream *st;
        st = avformat_new_stream(s, 0);
        if (!st)
            return AVERROR(ENOMEM);
        
        
        st->codec->codec_type = AV_PICTURE_TYPE_NONE;
        st->codec->codec_id   = AV_CODEC_ID_APNG;
        st->need_parsing      = AVSTREAM_PARSE_FULL_RAW;
        


        while(!url_feof(pb)) {
            
            uint32_t tag = avio_rb32(pb);
            
            switch(tag){
            
                case MKBETAG('a', 'c', 'T', 'L'):
                    {
                        st->nb_frames   = avio_rb32(pb);
                        apdc->nb_loops  = avio_rb32(pb);
                        ret             = 0;
                        break;
                        
                    }
                case MKBETAG('I', 'H', 'D', 'R'): 
                    {
                        apdc->width            = avio_rb32(pb);
                        apdc->height           = avio_rb32(pb);
                        apdc->bit_depth        = avio_rb32(pb);
                        apdc->color_type       = avio_rb32(pb);
                        apdc->compression_type = avio_rb32(pb);
                        apdc->filter_type      = avio_rb32(pb);
                        apdc->interlace_type   = avio_rb32(pb);
                         
                    }
            } 
            if (ret == 0)
                return ret;
        }

        return ret;  
}

AVInputFormat ff_apng_demuxer = {
    .name           = "apng",
    .raw_codec_id   = AV_CODEC_ID_APNG,
    .long_name      = NULL_IF_CONFIG_SMALL("Animated Portable Network Graphics"),
    .priv_data_size = sizeof(APNGDemuxContext),
    .read_probe     = apng_probe,
    .read_header    = read_header,
    .read_packet    = ff_raw_read_partial_packet,
    

};

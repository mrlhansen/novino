#include <kernel/term/psf.h>
#include <kernel/errno.h>

int psf_unicode(psf_t *psf, uint32_t unicode)
{
    uint32_t val, ch;
    int repl = -1;
    int num = 0;
    int seq = 0;

    if(psf->unicode == 0)
    {
        if(unicode < psf->num_glyph)
        {
            return unicode;
        }
        return -1;
    }

    if(psf->version == 1)
    {
        uint16_t *ptr = (uint16_t*)psf->unicode;
        while(num < psf->num_glyph)
        {
            val = *ptr++;
            if(val == 0xffff)
            {
                num++;
                seq = 0;
            }
            else if(val == 0xfffe)
            {
                seq = 1;
            }
            else if(seq == 0)
            {
                if(val == unicode)
                {
                    return num;
                }
                if(val == 0xfffd)
                {
                    repl = num;
                }
            }
        }
    }
    else
    {
        uint8_t *ptr = psf->unicode;
        while(num < psf->num_glyph)
        {
            ch = 0;
            val = *ptr++;

            if(val == 0xff)
            {
                num++;
                seq = 0;
                continue;
            }

            if(val == 0xfe)
            {
                seq = 1;
                continue;
            }

            if(seq)
            {
                continue;
            }

            if(val < 128)
            {
                ch = val;
            }
            else
            {
                int bit_mask = 0x3F;
                int bit_shift = 0;
                int bit_total = 0;
                int t = val;

                while((t & 0xC0) == 0xC0)
                {
                    val = *ptr++;
                    t = (t << 1) & 0xFF;
                    bit_total += 6;
                    bit_mask >>= 1;
                    bit_shift++;
                    ch = (ch << 6) | (val & 0x3F);
                }

                ch |= ((t >> bit_shift) & bit_mask) << bit_total;
            }

            if(ch == unicode)
            {
                return num;
            }
            if(ch == 0xfffd)
            {
                repl = num;
            }
        }
    }

    return repl;
}

int psf_parse(psf_t *psf, const void *data)
{
    psf1_t *psf1 = (void*)data;
    psf2_t *psf2 = (void*)data;

    if(psf1->magic == PSF1_FONT_MAGIC)
    {
        psf->version = 1;
        psf->height = psf1->height;
        psf->width = 8;
        psf->num_glyph = (psf1->flags & PSF1_FLAG_512) ? 512 : 256;
        psf->bytes_per_glyph = psf1->height;
        psf->bytes_per_row = 1;
        psf->padding = 0;
        psf->glyphs = (uint8_t*)psf1 + sizeof(psf1_t);
        psf->unicode = 0;

        if(psf1->flags & (PSF1_FLAG_SEQ | PSF1_FLAG_TAB))
        {
            psf->unicode = psf->glyphs + (psf->num_glyph * psf->bytes_per_glyph);
        }
    }
    else if(psf2->magic == PSF2_FONT_MAGIC)
    {
        psf->version = 2;
        psf->height = psf2->height;
        psf->width = psf2->width;
        psf->num_glyph = psf2->num_glyph;
        psf->bytes_per_glyph = psf2->bytes_per_glyph;
        psf->bytes_per_row = (psf->width + 7) / 8;
        psf->padding = (8 * psf->bytes_per_row) - psf->width;
        psf->glyphs = (uint8_t*)psf2 + psf2->header_size;
        psf->unicode = 0;

        if(psf2->flags)
        {
            psf->unicode = psf->glyphs + (psf->num_glyph * psf->bytes_per_glyph);
        }
    }
    else
    {
        return -EINVAL;
    }

    return 0;
}

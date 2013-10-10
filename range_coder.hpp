
#ifdef  ZP_RANGECODER_H
#error  Do not load me twice
#endif
#define ZP_RANGECODER_H

// Based on Eugene Shelwien's code from coders6c2.zip

#include "filer.hpp"
#include "common.hpp"

class RangeCoder {
    typedef unsigned int uint;

private:
    static const uint TOP =(1<<24);
    UINT64 low;
    uint  range, code;

    FilerLoad* m_in;
    FilerSave* m_out;

    // UCHAR *in_buf;
    // UCHAR *out_buf;

public:

    // void input (char *in)  { out_buf = in_buf = (UCHAR *)in;  }
    // void output(char *out) { in_buf = out_buf = (UCHAR *)out; }
    // int size_out(void) {return out_buf - in_buf;}

    // void StartEncode () {
    void init(FilerSave* f_out) { 
        m_out = f_out;
        m_in  = NULL;
        low=0;  
        range=(uint)-1; 
    }

    void init(FilerLoad* f_in) {
        m_in  = f_in;
        m_out = NULL;
        low=0;
        range=(uint)-1;
        code = 0;               // happy compiler
        for (int i=0; i<8; i++)
            code = (code<<8) | m_in->get();
    }

    RangeCoder(){}
    // void FinishEncode( void ) {
    ~RangeCoder() {
        if (m_out)
            for (int i=0; i<8; i++) {
                m_out->put(low >> 56);
                low <<= 8;
                // (*out_buf++ = low>>56), low<<=8; 
            }
    }

    // void FinishDecode( void ) {}

    void Encode (uint cumFreq, uint freq, uint totFreq) {
        low  += cumFreq * (range/= totFreq);
        range*= freq;

        assert (cumFreq + freq <= totFreq);

        while( range<TOP ) {
            if ( UCHAR((low^(low+range))>>56) ) 
                range = ((uint(low)|(TOP-1))-uint(low));
            // *out_buf++ = low>>56, range<<=8, low<<=8;
            m_out->put(low >> 56);
            range <<= 8;
            low   <<= 8;
        }
    }

    uint GetFreq (uint totFreq) {
        // shouldn't it be const?
        return code/(range/=totFreq);
    }

    void Decode (uint cumFreq, uint freq, uint totFreq) {
        uint temp = cumFreq*range;
        low  += temp;
        code -= temp;
        range*= freq;
 
        while( range<TOP ) {
            if ( UCHAR((low^(low+range))>>56) ) 
                range = ((uint(low)|(TOP-1))-uint(low));
            // code = (code<<8) | *in_buf++, range<<=8, low<<=8;
            code = (code<<8) | m_in->get();
            range<<=8;
            low  <<=8;
        }
    }
};

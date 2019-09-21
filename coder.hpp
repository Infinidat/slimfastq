/////////////////////////////////////////////////////////////////////////////
// This code was taken from fqz_comp. According to James Bonfield it is    //
// based on Eugene Shelwien's coders6c2.zip                                //
//                                                                         //
// (James admires the coder's efficiency, and so do I)                     //
//                                                                         //
// Also see                                                                //
//       http://cpansearch.perl.org/src/SALVA/Compress-PPMd-0.10/Coder.hpp //
/////////////////////////////////////////////////////////////////////////////


#ifdef  ZP_RANGECODER_H
#error  Do not load me twice
#endif
#define ZP_RANGECODER_H


#include "filer.hpp"
#include "common.hpp"

class RCoder {

private:
    enum { TOP =(1ULL<<24) };
    UINT64 low;
    UINT64 code;
    UINT32 range;

    FilerLoad* m_in;
    FilerSave* m_out;

public:
    void init(FilerSave* f_out) {
        m_out = f_out;
        m_in  = NULL;
        low=0;
        range=(UINT32)-1;
    }

    void init(FilerLoad* f_in) {
        m_in  = f_in;
        m_out = NULL;
        low=0;
        range=(UINT32)-1;
        code = 0;               // happy compiler
        for (int i=0; i<8; i++)
            code = (code<<8) | m_in->get();
    }

    ~RCoder() { done(); }
    void done() {
        // Allow explicit 'done' in case we wish not FilerSave close before this call
        if (m_out) {
            for (int i=0; i<8; i++) {
                m_out->put(low >> 56);
                low <<= 8;
            }
            m_out = NULL;
        }
    }

    ///////////////
    // FASTER !  //
    ///////////////
    void Encode (UINT32 cumFreq, UINT32 freq, UINT32 totFreq) {

        range /= totFreq ;
        low   += cumFreq * range ;
        range *= freq;

        // assert (cumFreq + freq <= totFreq);

        while( range<TOP ) {
            rarely_if ( (low^(low+range)) & (0xffULL<<56) )
                range = ((UINT32(low)|(TOP-1))-UINT32(low));
            m_out->put(low >> 56);
            range <<= 8;
            low   <<= 8;
        }
    }

    UINT32 GetFreq (UINT32 totFreq) {
        range /= totFreq;
        return code/range;
    }

    void Decode (UINT32 cumFreq, UINT32 freq, UINT32 totFreq) {
        UINT32 temp = cumFreq*range;
        low  += temp;
        code -= temp;
        range*= freq;

        while( range<TOP ) {
            rarely_if ( (low^(low+range)) & (0xffULL<<56) )
                range = ((UINT32(low)|(TOP-1))-UINT32(low));
            code <<=8;
            code |= m_in->get();
            range<<=8;
            low  <<=8;
        }
    }
};

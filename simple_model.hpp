
#ifdef  ZP_SIMPLE_MODULE_H
#error  Do not load me twice
#endif
#define ZP_SIMPLE_MODULE_H

#include "range_coder.hpp"

#define MAX_FREQ (1<<16)-32

template <int NSYM>
class SIMPLE_MODEL {
    enum { STEP=8 };

    UINT32 TotFreq;  // Total frequency
    UINT32 BubCnt;   // Periodic counter for bubble sort step

    // Array of Symbols approximately sorted by Freq. 
    struct SymFreqs {
        UINT16 Symbol;
        UINT16 Freq;
    } sentinel, F[NSYM+1];

protected:

    void normalize() {
        /* Faster than F[i].Freq for 0 <= i < NSYM */
        TotFreq=0;
        for (SymFreqs *s = F; s->Freq; s++) {
            s->Freq -= s->Freq>>1;
            TotFreq += s->Freq;
        }
    }

public:
    SIMPLE_MODEL() {}

    void init() {
        for ( int i=0; i<NSYM; i++ ) {
            F[i].Symbol = i;
            F[i].Freq   = 1;
        }

        TotFreq         = NSYM;
        sentinel.Symbol = 0;
        sentinel.Freq   = MAX_FREQ; // Always first; simplifies sorting.
        BubCnt          = 0;

        F[NSYM].Freq = 0; // terminates normalize() loop. See below.
    }

    inline void encodeSymbol(RangeCoder *rc, UINT16 sym) {
        SymFreqs *s = F;
        UINT32 AccFreq  = 0;

        while (s->Symbol != sym)
            AccFreq += s++->Freq;

        rc->Encode(AccFreq, s->Freq, TotFreq);
        s->Freq += STEP;
        TotFreq += STEP;

        if (TotFreq > MAX_FREQ)
            normalize();

        /* Keep approx sorted */
        if (((++BubCnt&15)==0) && s[0].Freq > s[-1].Freq) {
            SymFreqs t = s[0];
            s[0] = s[-1];
            s[-1] = t;
        }
    }

    inline UINT16 decodeSymbol(RangeCoder *rc) {
        SymFreqs* s = F;
        UINT32 freq = rc->GetFreq(TotFreq);
        UINT32 AccFreq;

        for (AccFreq = 0; (AccFreq += s->Freq) <= freq; s++);
        AccFreq -= s->Freq;

        rc->Decode(AccFreq, s->Freq, TotFreq);
        s->Freq += STEP;
        TotFreq += STEP;

        if (TotFreq > MAX_FREQ)
            normalize();

        /* Keep approx sorted */
        if (((++BubCnt&15)==0) && s[0].Freq > s[-1].Freq) {
            SymFreqs t = s[0];
            s[0] = s[-1];
            s[-1] = t;
            return t.Symbol;
        }

        return s->Symbol;
    }
};

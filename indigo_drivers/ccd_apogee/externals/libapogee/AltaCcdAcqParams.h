/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
*
* \class AltaCcdAcqParams 
* \brief derived class for managing the Alta's ADCs,
*         horizontal pattern files, and roi parameters 
* 
*/ 
#ifndef ALTACCDACQPARAMS_INCLUDE_H__ 
#define ALTACCDACQPARAMS_INCLUDE_H__ 

#include "CcdAcqParams.h" 
#include <string>

class AltaCcdAcqParams : public CcdAcqParams 
{ 
    public: 
        AltaCcdAcqParams(std::shared_ptr<CApnCamData> & camData,
                           std::shared_ptr<CameraIo> & camIo,  
                           std::shared_ptr<PlatformData> & platformData);

        virtual ~AltaCcdAcqParams(); 

        void Init();

       void SetResolution(Apg::Resolution res);

        void SetSpeed(Apg::AdcSpeed speed);

        void Set12BitGain(uint16_t gain);

        uint16_t Get12BitGain() {return m_Adc12BitGain;}

        void Set12BitOffset(uint16_t offset);

        uint16_t Get12BitOffset() {return m_Adc12BitOffset;}

        double Get16bitGain();

        bool IsAdsSimModeOn();

        void SetAdcGain(uint16_t gain, int32_t ad, int32_t channel);
        uint16_t GetAdcGain(int32_t ad, int32_t channel);
        void SetAdcOffset(uint16_t offset, int32_t ad, int32_t channel);
        uint16_t GetAdcOffset(int32_t ad, int32_t channel);
        uint16_t GetPixelShift();

    protected:
        uint16_t GetCcdImgRows() { return m_RoiNumRows; }
        uint16_t GetCcdImgCols() {  return m_RoiNumCols; }
        uint16_t GetCcdImgBinRows() { return m_NumRows2Bin; }
        uint16_t GetCcdImgBinCols() { return m_NumCols2Bin;}
        uint16_t GetTotalCcdCols();
        uint16_t CalcHPostRoiSkip(uint16_t HPreRoiSkip,
            uint16_t UnbinnedRoiCols);
        bool IsColCalcGood(uint16_t UnbinnedRoiCols, uint16_t PreRoiSkip, 
            uint16_t PostRoiSkip);

        CamCfg::APN_HPATTERN_FILE GetHPattern(Apg::AdcSpeed speed,
            CcdAcqParams::HPatternType ptype);

    private:
        std::string m_fileName;
        uint16_t m_Adc12BitGain;
        uint16_t m_Adc12BitOffset;

        void PrimeAdc();

        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        AltaCcdAcqParams(const AltaCcdAcqParams&);
        AltaCcdAcqParams& operator=(AltaCcdAcqParams&);

}; 

#endif

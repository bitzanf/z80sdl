#include "FTWrapper.hpp"

namespace std {
    std::error_code make_error_code(ftpp::FTError err) {
        return {err.err, ftpp::FTErrorCategory};
    }
}

namespace ftpp {
    void FTErrorCheck(FT_Error err) {
        if (err != 0) throw std::system_error{std::make_error_code(FTError(err))};
    }

    void FTErrorCheck(FT_Error err, const std::string &message) {
        if (err != 0) throw std::system_error{std::make_error_code(FTError(err)), message};
    }

    const char* FTError::getErrorMessage() const {
        #undef FTERRORS_H_
        #define FT_ERRORDEF( e, v, s )  case e: return s;
        #define FT_ERROR_START_LIST     switch (err) {
        #define FT_ERROR_END_LIST       }
        #include FT_ERRORS_H

        return "(Unknown error)";
    }
}
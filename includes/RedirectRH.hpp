#if !defined(__REDIRECT_RH_HPP__)
#define __REDIRECT_RH_HPP__

#include "ARequestHandler.hpp"

class RedirectRH : public ARequestHandler {
   private:
    static std::map<int, std::string> reason_phrases;

   public:
    RedirectRH(HttpRequest *request, FdManager &table);
    ~RedirectRH();

    virtual int respond();
    virtual void abort();

    static std::map<int, std::string> init_map();
};

#endif  // __REDIRECT_RH_HPP__

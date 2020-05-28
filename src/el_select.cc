//
// Created by liuliwu on 2020-05-28.
//

#include <sys/select.h>
#include <string>

namespace tLBS {
    class EventLoopSelect {
    private:
        fd_set readFds;
        fd_set writeFds;

    public:

        EventLoopSelect() {
            this->readFds = *((fd_set *) malloc(sizeof(fd_set)));
            this->writeFds = *((fd_set *) malloc(sizeof(fd_set)));
            FD_ZERO(&this->readFds);
            FD_ZERO(&this->writeFds);
        }

        ~EventLoopSelect() {
            free(&this->readFds);
            free(&this->writeFds);
        }

        int resize(int setSize) {
            if (setSize >= FD_SETSIZE) {
                return -1;
            }
            return 0;
        }

        int addEvent(int fd, int flags) {
            if (flags & EL_READABLE) {
                FD_SET(fd, &this->readFds);
            }
            if (flags & EL_WRITABLE) {
                FD_SET(fd, &this->writeFds);
            }
            return 0;
        }

        int delEvent(int fd, int flags) {
            if (flags & EL_READABLE) {
                FD_CLR(fd, &this->readFds);
            }
            if (flags & EL_WRITABLE) {
                FD_CLR(fd, &this->writeFds);
            }
            return 0;
        }

        static std::string getName() {
            return "select";
        }
    };
}

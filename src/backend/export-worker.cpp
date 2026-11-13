#include "backend/DetachedExport.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QCoreApplication::setApplicationName("VS Export Worker");
    const auto args=app.arguments();const int at=args.indexOf("--root");if(at<0||at+1>=args.size())return 2;
    const auto base=QDir(args[at+1]).absolutePath();if(!QDir().mkpath(base))return 3;
    QLockFile lock(QDir(base).filePath("worker.lock"));lock.setStaleLockTime(0);if(!lock.tryLock(0))return 0;
#ifdef Q_OS_WIN
    // UI death does not affect us. Worker death, however, must close BOTH children
    // rather than leave a muxer writing an output that a restarted job could reuse.
    HANDLE job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))||!AssignProcessToJobObject(job,GetCurrentProcess()))return 4;
    SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);
#endif
    vsr::exportjobs::Worker worker(base);return app.exec();
}

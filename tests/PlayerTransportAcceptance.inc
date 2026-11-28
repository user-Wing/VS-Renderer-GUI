void checkTrackPopupKeyboardAndTransportBounds() {
    PlayerWindow player;
    player.resize(1440,720);
    player.show();
    player.activateWindow();
    QTRY_VERIFY(player.isActiveWindow());
    player.setFocus();
    player.media_.insert("streams",QJsonArray{
        QJsonObject{{"type","video"},{"index",0}},QJsonObject{{"type","audio"},{"index",1}},
        QJsonObject{{"type","subtitle"},{"index",2}}});
    const auto press=[&](Qt::Key key) {
        QWidget *popup=QApplication::activePopupWidget();
        QTest::keyClick(popup ? popup : &player,key);
    };
    const auto active=[] {
        const auto *popup=QApplication::activePopupWidget();
        return popup ? popup->objectName() : QString();
    };
    for(auto [key,type]:{std::pair{Qt::Key_Z,"video"},std::pair{Qt::Key_X,"audio"},std::pair{Qt::Key_C,"subtitle"}}) {
        press(key);
        QTRY_COMPARE(active(),"playerTrackMenu-"+QString(type));
        press(key);
        QTRY_VERIFY(!QApplication::activePopupWidget());
    }
    press(Qt::Key_Z);
    QPointer<QWidget> previous=QApplication::activePopupWidget();
    QVERIFY(previous);
    press(Qt::Key_X);
    QTRY_COMPARE(active(),QString("playerTrackMenu-audio"));
    QVERIFY(!previous || !previous->isVisible());
    press(Qt::Key_C);
    QTRY_COMPARE(active(),QString("playerTrackMenu-subtitle"));
    press(Qt::Key_0);
    QTRY_VERIFY(!QApplication::activePopupWidget());

    auto *footer=player.findChild<QWidget*>("playerVolumeFooter");
    auto *files=player.findChild<QWidget*>("playerPreviousFile");
    QVERIFY(footer && files);
    for(int width:{1440,1114,1040,1039,870,810,730,670,540,505,470,1114,1440}) {
        player.resize(width,600);
        QTest::qWait(35);
        player.updateAlgorithmBadges();
        player.layoutChrome();
        QTRY_VERIFY(player.controls_->width()<=player.centralWidget()->width());
        const auto right=footer->mapToGlobal(footer->rect().topRight()).x();
        QVERIFY2(right<=player.centralWidget()->mapToGlobal(player.centralWidget()->rect().topRight()).x(),qPrintable(QString::number(width)));
        if(width==1114) {
            player.videoBadge_->setText("HEVC Main 10");
            player.audioBadge_->setText("TrueHD Atmos");
            QCoreApplication::processEvents();
            QTRY_VERIFY(files->isHidden());
            QTRY_VERIFY(player.controls_->width()<=player.centralWidget()->width());
        }
        if(width<1114)QVERIFY(files->isHidden());
        if(width==1440)QTRY_VERIFY(!files->isHidden());
    }
}

void checkPausedTransport() {
    QTemporaryDir directory;
    QString video=qEnvironmentVariable("VSR_TRANSPORT_MEDIA");
    if(video.isEmpty()){
        video=directory.filePath("transport.mkv");
        QProcess encode;
        encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=15",
             "-c:v","libx264","-threads","2","-g","24","-y",video});
        QVERIFY(encode.waitForFinished(20000));QCOMPARE(encode.exitCode(),0);
    }
    QFETCH(int,mode);
    QSettings settings(configPath(),QSettings::IniFormat);
    settings.setValue("decode/mode",mode);
    settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
    settings.setValue("basic/autoplay",true);settings.setValue("playback/remember",false);settings.sync();
    PlayerWindow player;player.resize(1440,810);player.show();player.activateWindow();
    const auto remembered=qEnvironmentVariable("VSR_TRANSPORT_POSITION").toLongLong();
    if(remembered>0) {
        player.settings_->setValue("playback/remember",true);
        player.history_->setValue("positions/"+QString::fromLatin1(QCryptographicHash::hash(video.toUtf8(),QCryptographicHash::Sha256).toHex()),remembered);
    }
    QVERIFY(player.openFile(video));
    const auto space=[&]{QTest::keyClick(&player,Qt::Key_Space);};
    const auto click=[&](const char *id){auto *button=player.findChild<QPushButton*>(id);QVERIFY(button);QTest::mouseClick(button,Qt::LeftButton);};
    const auto state=[&]{
        const auto s=player.snapshot();
        return QString("state=%1 pos=%2 pending=%3 seekTarget=%4 auto=%5 playing=%6 generation=%7/%8 frames=%9/%10 index=%11 error=%12")
            .arg(int(s.state)).arg(s.position100ns).arg(player.seekPending_).arg(player.seekUiTarget_).arg(player.autoPlay_).arg(player.playing_)
            .arg(s.timelineGeneration).arg(player.generation_).arg(s.presentedVideoFrames).arg(player.seekPresented_).arg(s.frameIndex).arg(player.clock_->lastError());
    };
    QTRY_VERIFY2_WITH_TIMEOUT(player.playing_ && player.snapshot().presentedVideoFrames>2,qPrintable(state()),15000);
    space();QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Paused,qPrintable(state()),2500);
    const auto paused=player.position();
    space();QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing && player.position()>paused+2000000,qPrintable(state()),5000);
    for(int round=0;round<3;++round){
        space();QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Paused,qPrintable(state()),2500);
        QList<const char*> buttons{"playerNextFrame","playerPreviousFrame","playerForwardSecond","playerBackwardSecond","playerNextKeyframe","playerPreviousKeyframe"};
        std::rotate(buttons.begin(),buttons.begin()+2*round,buttons.end());
        for(auto id:buttons){
            const auto before=player.snapshot();
            click(id);
            QTRY_VERIFY2_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().presentedVideoFrames>before.presentedVideoFrames,
                qPrintable(QString(id)+" "+state()),6000);
        }
        auto *slider=player.timeline_;
        QTest::mouseClick(slider,Qt::LeftButton,Qt::NoModifier,QPoint(slider->width()*(1+round)/5,slider->height()/2));
        const auto target=player.snapshot().duration100ns*slider->value()/100000;
        QTRY_VERIFY2_WITH_TIMEOUT(!player.seekPending_ && qAbs(player.position()-target)<1000000,qPrintable(state()),7000);
        if(round%2)click("playerPlay");else space();
        QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing && player.position()>target+2000000,qPrintable(state()),5000);
    }
    space();QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
    player.seekTime(0);
    QTRY_VERIFY2_WITH_TIMEOUT(!player.seekPending_ && player.position()<1000000,qPrintable(state()),5000);
    click("playerPreviousKeyframe");
    QTest::qWait(100);
    space();
    QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing && player.position()>2000000,qPrintable(state()),5000);

}

void exactSeekTargetsData() {
    QTest::addColumn<QString>("operation");
    QTest::addColumn<int>("mode");
    for(int mode:{1,2})for(const QString operation:{"frame","time","burst","mixed"})
        QTest::newRow(qPrintable(operation+(mode==1?"-SW":"-HW")))<<operation<<mode;
}

void checkExactSeekTargets() {
    QFETCH(QString,operation);
    QFETCH(int,mode);
    QTemporaryDir directory;
    auto video=qEnvironmentVariable("VSR_TRANSPORT_MEDIA");
    const bool generated=video.isEmpty();
    if(generated){
        video=directory.filePath("exact-seek.mkv");
        QProcess encode;
        encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=30:duration=22",
             "-c:v","libx264","-threads","2","-g","240","-sc_threshold","0","-y",video});
        QVERIFY(encode.waitForFinished(20000));QCOMPARE(encode.exitCode(),0);
    }
    QSettings settings(configPath(),QSettings::IniFormat);
    settings.setValue("decode/mode",mode);settings.setValue("basic/autoplay",true);
    settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
    settings.setValue("playback/remember",false);settings.sync();
    PlayerWindow player;player.resize(1440,810);player.show();
    QVERIFY(player.openFile(video));
    QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && player.snapshot().presentedVideoFrames>2,10000);
    QTest::keyClick(&player,Qt::Key_Space);
    QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
    QTest::qWait(450); // Exercise the first deliberate seek after key repeat has stopped.
    const auto before=player.snapshot();
    QVERIFY(player.direct_);
    const auto frame=qEnvironmentVariableIntValue("VSR_SEEK_TEST_FRAME")>0?qEnvironmentVariableIntValue("VSR_SEEK_TEST_FRAME"):441;
    const auto timestamp=qEnvironmentVariableIntValue("VSR_SEEK_TEST_MS")>0?qint64(qEnvironmentVariableIntValue("VSR_SEEK_TEST_MS"))*10000:153000000;
    QElapsedTimer elapsed;elapsed.start();
    if(operation=="frame") {
        player.frame_->setText(QString::number(frame));player.frameEditPending_=true;
        QTest::keyClick(player.frame_,Qt::Key_Return);
        QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().frameIndex==frame && !player.seekPending_,
            qPrintable(QString("frame=%1 requested=%2 pending=%3 position=%4 presented=%5 total=%6")
                .arg(player.snapshot().frameIndex).arg(frame).arg(player.seekPending_).arg(player.position())
                .arg(player.snapshot().presentedVideoFrames-before.presentedVideoFrames).arg(player.clip_.totalFrames)),12000);
    } else {
        auto target=timestamp;
        if(operation=="time")player.seekTime(target);
        else {
            auto* forward=player.findChild<QPushButton*>("playerForwardSecond");
            auto* back=player.findChild<QPushButton*>("playerBackwardSecond");
            QVERIFY(forward && back);
            target=before.position100ns;
            for(int i=0;i<6;++i){
                const bool reverse=operation=="mixed" && (i==2 || i==4);
                (reverse?back:forward)->click();
                target+=reverse?-10000000:10000000;
                if(operation=="mixed" && i<5)QTest::qWait(12);
            }
        }
        QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.queuedSeek_<0,8000);
        QVERIFY2(qAbs(player.snapshot().position100ns-target)<500000,qPrintable(QString("target=%1 actual=%2").arg(target).arg(player.snapshot().position100ns)));
    }
    const auto after=player.snapshot();
    qInfo()<<operation<<"decoder"<<after.decodeMode<<"ms"<<elapsed.elapsed()<<"pts"<<after.framePts<<"presented"<<after.presentedVideoFrames-before.presentedVideoFrames;
    if(operation=="frame" && qEnvironmentVariableIsSet("VSR_SEEK_TEST_PTS"))
        QCOMPARE(after.framePts,qEnvironmentVariable("VSR_SEEK_TEST_PTS").toLongLong());
    if(operation!="mixed")QCOMPARE(after.presentedVideoFrames-before.presentedVideoFrames,quint64(1));
    else QVERIFY(after.presentedVideoFrames>before.presentedVideoFrames);
    if(generated && operation=="frame"){
        player.frame_->setText("10000");player.frameEditPending_=true;
        QTest::keyClick(player.frame_,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().frameIndex==659,6000);
        QCOMPARE(player.snapshot().framePts,qint64(21967));
        QCOMPARE(player.snapshot().presentedVideoFrames-after.presentedVideoFrames,quint64(1));
    }
}

void keyTransportData() {
    QTest::addColumn<QString>("operation");
    QTest::addColumn<int>("mode");
    for(int mode:{1,2})for(const QString operation:{"playing","pausedKeys","seconds","playingSeconds","keyThenSecond","resume"})
        QTest::newRow(qPrintable(operation+(mode==1?"-SW":"-HW")))<<operation<<mode;
}

void checkKeyTransport() {
    QFETCH(QString,operation);
    QFETCH(int,mode);
    QTemporaryDir dir;
    auto source=qEnvironmentVariable("VSR_TRANSPORT_MEDIA");
    qint64 first=2000,second=4000;
    if(source.isEmpty()){
        source=dir.filePath("keys.mkv");
        QProcess encode;
        encode.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=160x90:rate=30:duration=9",
             "-c:v","libx264","-threads","2","-g","60","-sc_threshold","0","-y",source});
        QVERIFY(encode.waitForFinished(20000));QCOMPARE(encode.exitCode(),0);
    }else{
        first=qEnvironmentVariableIntValue("VSR_KEY_FIRST_MS");second=qEnvironmentVariableIntValue("VSR_KEY_SECOND_MS");
        QVERIFY(first>0 && second>first);
    }
    QSettings settings(configPath(),QSettings::IniFormat);
    settings.setValue("decode/mode",mode);settings.setValue("basic/autoplay",true);
    settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));
    settings.setValue("playback/remember",false);settings.sync();
    PlayerWindow player;player.resize(1440,810);player.show();
    QVERIFY(player.openFile(source));
    QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && player.snapshot().presentedVideoFrames>2,12000);
    const QFileInfo video(source);
    if(operation=="resume" && QFileInfo::exists(video.dir().filePath(video.completeBaseName()+".mka")))
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().isExternalAudio,3000);
    const bool playing=operation=="playing" || operation=="playingSeconds";
    if(!playing){
        QTest::keyClick(&player,Qt::Key_Space);
        QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);
    }
    const bool seconds=operation=="seconds" || operation=="playingSeconds";
    auto* button=player.findChild<QPushButton*>(seconds?"playerForwardSecond":"playerNextKeyframe");
    QVERIFY(button && button->isVisible());
    auto* window=button->window()->windowHandle();
    const auto point=window->mapFromGlobal(button->mapToGlobal(button->rect().center()));
    int clicks=0;connect(button,&QPushButton::clicked,&player,[&]{++clicks;});
    const auto before=player.snapshot();
    const bool singleKey=operation=="resume" || operation=="keyThenSecond";
    if(singleKey)QTest::mouseClick(window,Qt::LeftButton,{},point);
    else QTest::mouseDClick(window,Qt::LeftButton,{},point);
    QCOMPARE(clicks,singleKey?1:2);
    if(operation=="keyThenSecond"){
        QTest::mouseClick(player.findChild<QPushButton*>("playerForwardSecond"),Qt::LeftButton);
        QTRY_VERIFY2_WITH_TIMEOUT(!player.seekPending_ && qAbs(player.snapshot().framePts-first-1000)<50,
            qPrintable(QString("key+1s actual=%1").arg(player.snapshot().framePts)),4000);
        return;
    }
    if(seconds){
        const auto target=before.position100ns+20000000;
        QTRY_VERIFY2_WITH_TIMEOUT(!player.seekPending_ && player.snapshot().framePts*10000>=target-500000 &&
            player.snapshot().framePts*10000<target+(playing?8000000:500000),
            qPrintable(QString("second target=%1 actual=%2").arg(target).arg(player.snapshot().framePts)),5000);
        QCOMPARE(player.playing_,playing);
        if(!playing){
            auto* back=player.findChild<QPushButton*>("playerBackwardSecond");
            QVERIFY(back && back->isVisible());
            QTest::mouseDClick(window,Qt::LeftButton,{},window->mapFromGlobal(back->mapToGlobal(back->rect().center())));
            QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && qAbs(player.snapshot().position100ns-before.position100ns)<1000000,4000);
            QCOMPARE(player.snapshot().state,ThreeFpState::Paused);
        }
        return;
    }
    if(operation=="playing")QVERIFY(player.playing_);
    const auto expected=operation=="resume"?first:second;
    QTRY_VERIFY2_WITH_TIMEOUT(player.snapshot().framePts>=expected && player.snapshot().framePts<expected+1000,
        qPrintable(QString("expected=%1 actual=%2").arg(expected).arg(player.snapshot().framePts)),4000);
    if(operation=="playing"){
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Playing && player.snapshot().framePts>expected+150,2000);
        return;
    }
    QCOMPARE(player.snapshot().state,ThreeFpState::Paused);
    QCOMPARE(player.snapshot().framePts,expected);
    if(operation=="pausedKeys"){
        auto* previous=player.findChild<QPushButton*>("playerPreviousKeyframe");
        QVERIFY(previous);
        QTest::mouseClick(previous,Qt::LeftButton);
        QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().framePts,first,3000);
        QCOMPARE(player.snapshot().state,ThreeFpState::Paused);
    }
    if(operation=="resume"){
        const auto displayed=player.snapshot().presentedVideoFrames;
        QElapsedTimer timer;timer.start();QTest::keyClick(&player,Qt::Key_Space);
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>displayed && player.snapshot().framePts>expected,4000);
        qInfo()<<"resume after keyframe ms"<<timer.elapsed()<<"pts"<<player.snapshot().framePts<<"from"<<expected;
        QVERIFY(player.snapshot().framePts<expected+900);
        QVERIFY2(timer.elapsed()<1200,"Playback did not resume promptly after the keyframe was visible");
    }
}

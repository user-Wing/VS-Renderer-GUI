    void runStartupTheme(){
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        QImage background(16,16,QImage::Format_RGB32);background.fill(Qt::green);QVERIFY(background.save(directory.filePath("green.png")));
        QSettings skin(directory.filePath("themes.ini"),QSettings::IniFormat);skin.setValue("VSP-Main/image","green.png");skin.setValue("VSP-Main/transparency",0);skin.setValue("VSP-Main/brightness",180);skin.sync();
        PlayerWindow player;player.show();QVERIFY(QTest::qWaitForWindowExposed(&player));QTest::qWait(100);
        auto *placeholder=player.pane_->findChild<QLabel *>("previewPlaceholder");QVERIFY(placeholder);QVERIFY(placeholder->isVisible());
        QCOMPARE(placeholder->grab().toImage().pixelColor(4,4),QColor(Qt::green));
        const auto visible=player.pane_->grab().toImage();const auto point=placeholder->mapTo(player.pane_,QPoint(4,4))*visible.devicePixelRatio();
        visible.save(QDir(QCoreApplication::applicationDirPath()).filePath("startup-player-background.png"));
        QCOMPARE(visible.pixelColor(point),QColor(Qt::green));
    }
    void runInfoFpsTracksNativePlayback(){
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        const auto source=directory.filePath("fps.mkv");QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24:duration=8","-c:v","ffv1",source});QVERIFY(encoder.waitForFinished(15000));QCOMPARE(encoder.exitCode(),0);
        PlayerWindow player;player.settings_->setValue("basic/autoplay",false);player.settings_->setValue("playback/remember",false);player.show();player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));QVERIFY(player.openFile(source));QVERIFY(!player.findChild<QWidget *>("playerIdleGuide")->isVisible());QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_,15000);
        QVERIFY(!player.playing_);QVERIFY(player.clock_->play());QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,10000);
        player.infoVisible_=true;player.infoFpsTimer_.invalidate();player.updateInfo();QTest::qWait(1500);player.updateInfo();
        QVERIFY(player.infoCurrentFps_>5);qInfo()<<"Native state"<<int(player.snapshot().state)<<"UI playing"<<player.playing_<<"measured"<<player.infoCurrentFps_<<player.info_->text();
        QVERIFY(!player.info_->text().contains(QStringLiteral("当前帧率：0.00")));
        QVERIFY(!player.info_->text().contains(QChar(0xff08)));QVERIFY(!player.info_->text().contains(QChar(0xff09)));
        QVERIFY(!player.info_->text().contains(QStringLiteral("实际视频链路")));QVERIFY(!player.info_->text().contains(QStringLiteral("系统首个GPU")));
        QVERIFY(player.clock_->pause());QTRY_COMPARE(player.snapshot().state,ThreeFpState::Paused);player.updateInfo();QVERIFY(player.info_->text().contains(QStringLiteral("当前帧率：0.00")));
    }
    void runBuiltin1080To2160(){
        const auto source=qEnvironmentVariable("VSR_1080_48_SOURCE");
        if(source.isEmpty())QSKIP("Set VSR_1080_48_SOURCE for the bounded 1080p48 to 2160p48 benchmark");
        const int stage=qEnvironmentVariableIntValue("VSR_1080_48_STAGE");QVERIFY(stage>=0&&stage<=10);
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        PlayerWindow player;player.settings_->setValue("basic/autoplay",false);player.settings_->setValue("playback/remember",false);player.settings_->setValue("subtitle/visible",false);player.settings_->setValue("player/muted",true);player.settings_->setValue("render/shaderDirectory",directory.filePath("shaders"));
        player.resize(1280,800);player.show();const double dpi=player.pane_->devicePixelRatioF();player.pane_->surface()->setFixedSize(qRound(3840/dpi),qRound(2160/dpi));
        const QStringList files{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy","Anime-8-A-CNN-Enhanced.vpy","Anime-9-A-CNN.vpy","MoeSR-50K.vpy"};
        player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/"+files[stage]));QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_&&player.snapshot().presentedVideoFrames>0,30000);
        QCOMPARE(player.snapshot().videoWidth,1920u);QCOMPARE(player.snapshot().videoHeight,1080u);
        player.seekTime(600000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_&&player.position()>590000000,30000);
        const auto reference=qEnvironmentVariable("VSR_1080_48_REFERENCE");
        if(!reference.isEmpty()){
            if(reference=="native-1x"){player.customShaderPackage_.clear();QVERIFY(player.clock_->setCustomShader({}));QVERIFY(player.clock_->setScalingAlgorithms(static_cast<ThreeFpScalingAlgorithm>(stage==6?13:14),ThreeFpScalingAlgorithm::Jinc2));}
            else {player.customShaderPackage_=reference;QVERIFY(player.clock_->setCustomShader(reference,3840,2160));}
        }else if(!player.customShaderPackage_.isEmpty())QVERIFY(player.clock_->setCustomShader(player.customShaderPackage_,3840,2160));
        if(player.snapshot().state!=ThreeFpState::Playing)QVERIFY(player.clock_->play());QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Playing,30000);QTest::qWait(2500);
        const auto first=player.snapshot();QElapsedTimer wall;wall.start();QTest::qWait(qMax(6,qEnvironmentVariableIntValue("VSR_1080_48_SECONDS"))*1000);const auto last=player.snapshot();
        const auto physical=player.pane_->surface()->size()*player.pane_->devicePixelRatioF();QCOMPARE(physical,QSize(3840,2160));
        qInfo().noquote()<<"builtin-benchmark"<<QString::fromUtf8(QJsonDocument(QJsonObject{{"stage",stage},{"reference",reference},{"wallMs",double(wall.elapsed())},{"presented",double(last.presentedVideoFrames-first.presentedVideoFrames)},{"decoded",double(last.decodedVideoFrames-first.decodedVideoFrames)},{"dropped",double(last.droppedVideoFrames-first.droppedVideoFrames)},{"fps",(last.presentedVideoFrames-first.presentedVideoFrames)*1000./wall.elapsed()},{"width",physical.width()},{"height",physical.height()},{"customPackage",player.customShaderPackage_},{"scaling",int(last.videoScalingMode)}}).toJson(QJsonDocument::Compact));
    }
    void runHlslOptimizationAcceptance(){
        const auto source=qEnvironmentVariable("VSR_NUX_SOURCE"),evidence=qEnvironmentVariable("VSR_HLSL_EVIDENCE");
        if(source.isEmpty() || evidence.isEmpty())QSKIP("Set VSR_NUX_SOURCE and VSR_HLSL_EVIDENCE for isolated local acceptance");
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");
        qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        QDir().mkpath(evidence);
        PlayerWindow player;player.settings_->setValue("basic/autoplay",false);player.settings_->setValue("playback/remember",false);
        player.settings_->setValue("player/muted",true);player.settings_->setValue("subtitle/visible",false);
        player.settings_->setValue("render/shaderDirectory",directory.filePath("shaders"));
        player.settings_->setValue("render/secondUpscale",-1);player.settings_->setValue("render/hlsl",false);
        const QDir builtin(QDir(PresetStore::directory()).filePath("builtin"));
        player.resize(1280,800);player.show();player.loadPreset(builtin.filePath("Anime-5-D3D11.vpy"));
        QVERIFY2(player.openFile(source),qPrintable(player.message_->text()));
        QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.snapshot().presentedVideoFrames>0,30000);
        const QStringList files{"Anime-0-CNN-Enhanced.vpy","Anime-1-CNN.vpy","Anime-2-No-CNN-Enhanced.vpy","Anime-3-No-CNN.vpy","Anime-4-Jinc.vpy","Anime-5-D3D11.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy","Anime-8-A-CNN-Enhanced.vpy","Anime-9-A-CNN.vpy","MoeSR-50K.vpy"};
        QImage oneXJinc;
        for(int stage:{10,8,9,0,1,2,3,6,7,5}){
            player.clock_->setView(stage==6 || stage==7?2.f:1.f,0,0);
            player.loadPreset(builtin.filePath(files[stage]));QCOMPARE(player.qualityStage_,stage);QVERIFY(player.direct_);
            const bool packaged=stage>=8 || stage==6 || stage==7;
            QCOMPARE(!player.customShaderPackage_.isEmpty(),packaged);
            if(stage==6 || stage==7){QCOMPARE(player.scalingAlgorithm(),stage==6?4:7);QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),-1);}
            if(stage==10 || stage==6 || stage==7){
                QFile embedded(stage==10?":/filters/builtin-shaders/moesr-50k.hlsl.vsrhlsl":":/filters/builtin-shaders/anime4k-1x.glsl.vsrhlsl");
                QFile materialized(player.customShaderPackage_);QVERIFY(embedded.open(QIODevice::ReadOnly));QVERIFY(materialized.open(QIODevice::ReadOnly));
                QCOMPARE(materialized.readAll(),embedded.readAll());
            }
            player.seekTime(30000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.position()>=29000000,30000);
            const auto image=player.clock_->capture();QVERIFY(!image.isNull());
            QVERIFY(!player.hlslFailed_);QVERIFY2(player.clock_->lastError().isEmpty(),qPrintable(player.clock_->lastError()));
            if(stage==6)oneXJinc=image;
            if(stage==7)QVERIFY2(image!=oneXJinc,"1X Jinc and D3D11 must retain distinct final upscaling");
            int lo=255,hi=0;for(int y=image.height()/4;y<image.height()*3/4;y+=13)for(int x=image.width()/4;x<image.width()*3/4;x+=13){const int v=qGray(image.pixel(x,y));lo=std::min(lo,v);hi=std::max(hi,v);}
            QVERIFY2(hi-lo>32,"Rendered output must contain spatial picture detail");
            QVERIFY(image.save(QDir(evidence).filePath(QString("stage-%1.png").arg(stage))));
            player.togglePlayback();QTRY_VERIFY_WITH_TIMEOUT(player.playing_,10000);
            const auto frames=player.snapshot().presentedVideoFrames;QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>frames+2,30000);
            player.togglePlayback();QTRY_COMPARE_WITH_TIMEOUT(player.snapshot().state,ThreeFpState::Paused,10000);
            qInfo()<<"HLSL acceptance"<<stage<<"algorithm"<<player.scalingAlgorithm()<<"picture-range"<<lo<<hi<<"package"<<player.customShaderPackage_;
        }
        // An invalid 1X package must keep the original native 1X algorithm.
        player.loadPreset(builtin.filePath(files[6]));QVERIFY(!player.customShaderPackage_.isEmpty());
        player.customShaderPackage_=directory.filePath("missing.vsrhlsl");player.applyScaling();
        QVERIFY(player.customShaderPackage_.isEmpty());QVERIFY(player.hlslFailed_);QCOMPARE(player.scalingAlgorithm(),13);
        player.loadPreset(builtin.filePath(files[7]));QVERIFY(!player.customShaderPackage_.isEmpty());QCOMPARE(player.scalingAlgorithm(),7);
        player.loadPreset(builtin.filePath(files[1]));QVERIFY(player.customShaderPackage_.isEmpty());QCOMPARE(player.scalingAlgorithm(),9);
    }

    void runAbLoopPlayback(bool useVs=false) {
        QTemporaryDir directory;const auto source=directory.filePath("loop.mkv");
        QProcess ff;ff.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=3","-c:v","ffv1","-y",source});
        QVERIFY(ff.waitForFinished(15000));QCOMPARE(ff.exitCode(),0);
        QSettings settings(configPath(),QSettings::IniFormat);settings.setValue("basic/autoplay",false);settings.setValue("playback/remember",false);settings.sync();
        PlayerWindow player;player.resize(1100,700);player.show();
        if(useVs){QFile preset(directory.filePath("identity.vpy"));QVERIFY(preset.open(QIODevice::WriteOnly));preset.write(VpyScriptBuilder::build(source,SourceFilter::Native,FilterGraph()).script.toUtf8());preset.close();player.loadPreset(preset.fileName());}
        QVERIFY(player.openFile(source));
        QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.outputSnapshot().presentedVideoFrames>0 && !player.seekPending_,15000);
        QCOMPARE(player.direct_,!useVs);
        player.seekTime(5000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.seekUiTarget_<0,5000);
        {QMenu menu;player.addLoopMenu(&menu);auto *a=menu.findChild<QAction*>("playerLoopIn");QVERIFY(a);a->trigger();}
        QVERIFY(qAbs(player.loopIn_-5000000)<500000);
        player.seekTime(10000000);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.seekUiTarget_<0,5000);
        {QMenu menu;player.addLoopMenu(&menu);auto *b=menu.findChild<QAction*>("playerLoopOut");QVERIFY(b&&b->isEnabled());b->trigger();}
        {QMenu menu;player.addLoopMenu(&menu);QVERIFY(menu.findChild<QAction*>("playerLoopRenderer")->isEnabled());}
        QVERIFY(player.loopEnabled_);const auto end=player.loopOut_;player.setLoopPoint(true,0);QCOMPARE(player.loopOut_,end);
        player.seekTime(player.loopIn_);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.seekUiTarget_<0,5000);
        const auto generation=player.snapshot().timelineGeneration;player.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>=generation+3,10000);
        QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && !player.seekPending_,5000);player.togglePlayback();
        const auto paused=player.snapshot().timelineGeneration;QTest::qWait(800);QCOMPARE(player.snapshot().timelineGeneration,paused);QVERIFY(!player.playing_);
        // EOF must restart too, rather than stopping before the loop check.
        player.setLoopPoint(false,24000000);player.setLoopPoint(true,player.snapshot().duration100ns);
        player.seekTime(player.loopIn_);QTRY_VERIFY_WITH_TIMEOUT(!player.seekPending_ && player.seekUiTarget_<0,5000);
        const auto eofGeneration=player.snapshot().timelineGeneration;player.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().timelineGeneration>=eofGeneration+2,10000);
        QTRY_VERIFY_WITH_TIMEOUT(player.playing_ && !player.seekPending_,5000);
        {QMenu menu;player.addLoopMenu(&menu);menu.findChild<QAction*>("playerLoopClear")->trigger();}
        QVERIFY(!player.loopEnabled_);QCOMPARE(player.loopIn_,qint64(-1));
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().state==ThreeFpState::Ended,7000);
        player.setLoopPoint(false,0);player.setLoopPoint(true,10000000);QVERIFY(player.openFile(source));QVERIFY(!player.loopEnabled_);QCOMPARE(player.loopOut_,qint64(-1));
    }
    void runThumbnailRegistrationRestoresPrevious() {
        const auto root="HKEY_CURRENT_USER\\Software\\VSRendererTests\\Thumb-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSettings cleanup(root,QSettings::NativeFormat);const auto remove=qScopeGuard([&]{cleanup.clear();cleanup.sync();});
        QSettings classes(root+"\\Classes",QSettings::NativeFormat);
        const QString key=".jxl/shellex/{E357FCCD-A995-4576-B01F-234630154E96}/.";
        const QString legacy=".jxl/shellex/{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}/.";
        classes.setValue(legacy,"old-legacy");
        classes.setValue(key,"old-handler");classes.setValue(".jxl/.","OldViewer");classes.sync();
        QTemporaryDir dir;QFile dll(dir.filePath("provider.dll"));QVERIFY(dll.open(QIODevice::WriteOnly));dll.write("fixture");dll.close();
        QVERIFY(registerThumbnailProvider({"jxl","mkv","qoi","jp2","j2k","exr"},dll.fileName(),root));
        for(const auto &extension:QStringList{"qoi","jp2","j2k","exr"})QVERIFY(playerImageExtensions().contains(extension));
        QVERIFY(registerThumbnailProvider({"jxl"},dll.fileName(),root));
        classes.sync();QVERIFY(classes.value(key).toString()!="old-handler");QCOMPARE(classes.value(".jxl/.").toString(),QString("OldViewer"));
        QCOMPARE(classes.value(legacy),classes.value(key));
        QVERIFY(unregisterThumbnailProvider(root));classes.sync();QCOMPARE(classes.value(key).toString(),QString("old-handler"));
        QCOMPARE(classes.value(legacy).toString(),QString("old-legacy"));
        QVERIFY(!classes.contains("SystemFileAssociations/.jxl/shellex/{BB2E617C-0920-11D1-9A0B-00C04FC2D6C1}/."));
        QVERIFY(!classes.contains(".mkv/shellex/{E357FCCD-A995-4576-B01F-234630154E96}/."));
        QVERIFY(!registerThumbnailProvider({"../bad"},dll.fileName(),root));
        QVERIFY(registerThumbnailProvider({"jxl"},dll.fileName(),root));classes.setValue(key,"new-owner");classes.sync();
        QVERIFY(unregisterThumbnailProvider(root));classes.sync();QCOMPARE(classes.value(key).toString(),QString("new-owner"));
    }

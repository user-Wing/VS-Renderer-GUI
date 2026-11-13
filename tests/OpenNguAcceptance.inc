    void runOpenNguMenu(){
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        PlayerWindow player;player.showContextMenu(QPoint(50,50));
        auto *menu=player.findChild<QMenu*>("playerContextMenu");QVERIFY(menu);
        auto *primary=menu->findChild<QMenu*>("playerPassthroughFilters");auto *second=menu->findChild<QMenu*>("playerSecondUpsampling");auto *down=menu->findChild<QMenu*>("playerDownsampling");QVERIFY(primary && second && down);
        const QStringList primaryIds{"playerPrimaryScale15","playerPrimaryScale4","playerPrimaryScale7"};
        const QStringList secondIds{"playerSecondScale4","playerSecondScale7","playerSecondScale15"};
        for(int i=0;i<3;++i){QCOMPARE(primary->actions()[i]->objectName(),primaryIds[i]);QCOMPARE(second->actions()[i]->objectName(),secondIds[i]);}
        QVERIFY(!down->findChild<QAction*>("playerDownscale15"));
        QCOMPARE(player.settings_->value("render/secondUpscale",4).toInt(),4);
        primary->actions().first()->trigger();QCOMPARE(player.settings_->value("render/upscale").toInt(),15);
        second->actions()[2]->trigger();QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),15);menu->close();
    }
    void runOpenNguPlayback(){
        QTemporaryDir directory;const auto previous=qgetenv("VSR_PLAYER_CONFIG");qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(previous.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",previous);});
        const auto path=QDir(QCoreApplication::applicationDirPath()).filePath("shaders/OpenNGU-AA-Medium-BestMatch-2x-SDR.hlsl");if(!QFileInfo::exists(path))QSKIP("OpenNGU private shader package is supplied separately");
        const auto source=directory.filePath("ngu.mkv");QProcess encoder;encoder.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-f","lavfi","-i","testsrc2=size=128x72:rate=24:duration=3","-c:v","ffv1","-pix_fmt","yuv444p10le","-colorspace","bt709","-color_primaries","bt709","-color_trc","bt709",source});QVERIFY(encoder.waitForFinished(15000));QCOMPARE(encoder.exitCode(),0);
        PlayerWindow player;player.settings_->setValue("basic/autoplay",false);player.settings_->setValue("playback/remember",false);player.settings_->setValue("render/upscale",15);player.settings_->setValue("render/secondUpscale",4);
        player.show();player.pane_->surface()->setFixedSize(256,144);player.settings_->setValue("render/upscale",15);QVERIFY(player.openFile(source));QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_,15000);
        QTRY_VERIFY_WITH_TIMEOUT(player.openNguActive_,5000);QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,5000);
        player.clock_->redraw();QTest::qWait(100);const auto ngu=player.clock_->capture();QVERIFY(!ngu.isNull());QCOMPARE(player.snapshot().version,23u);
        player.settings_->setValue("render/upscale",4);player.applyScaling();player.clock_->redraw();QTest::qWait(150);const auto jinc=player.clock_->capture();QVERIFY(!jinc.isNull());QVERIFY2(ngu!=jinc,"OpenNGU must change the real presented pixels");
        player.settings_->setValue("render/upscale",15);player.settings_->setValue("render/openNguShader",directory.filePath("missing.hlsl"));player.applyScaling();QVERIFY(!player.openNguActive_);player.clock_->redraw();QTest::qWait(150);QVERIFY(!player.clock_->capture().isNull());QVERIFY(player.hlslFallback_.startsWith("OpenNGU"));
        player.settings_->remove("render/openNguShader");player.settings_->setValue("render/upscale",4);player.settings_->setValue("render/secondUpscale",15);player.applyScaling();QVERIFY(player.openNguActive_);player.clock_->redraw();QTest::qWait(150);const auto second=player.clock_->capture();QVERIFY(!second.isNull());QVERIFY(second!=jinc);
        // A 1x preceding shader must be preserved while OpenNGU handles the remaining scale.
        const auto identity=directory.filePath("identity.glsl");QFile shader(identity);QVERIFY(shader.open(QIODevice::WriteOnly));shader.write("//!HOOK MAIN\n//!BIND HOOKED\n//!WIDTH NATIVE.w\n//!HEIGHT NATIVE.h\nvec4 hook(){return HOOKED_tex(HOOKED_pos);}\n");shader.close();QVERIFY(player.importCustomShader(identity));
        player.applyScaling();QVERIFY(player.openNguActive_);QVERIFY(!player.customShaderPackage_.isEmpty());player.clock_->redraw();QTest::qWait(150);const auto chained=player.clock_->capture();QVERIFY(!chained.isNull());
        player.settings_->setValue("render/secondUpscale",4);player.applyScaling();player.clock_->redraw();QTest::qWait(150);const auto base=player.clock_->capture();QVERIFY(!base.isNull());QVERIFY(base!=chained);
    }

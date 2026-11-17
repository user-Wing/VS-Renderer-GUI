    void runAnimeSixMigration(){
        QTemporaryDir directory;const auto old=qgetenv("VSR_PLAYER_CONFIG");
        qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(old.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",old);});
        {QSettings settings(directory.filePath("player.ini"),QSettings::IniFormat);settings.setValue("player/animeStage",2);settings.setValue("player/animeStages",QStringList{"2","3","6","7","5"});settings.setValue("player/preset",QDir(PresetStore::directory()).filePath("builtin/Anime-2-No-CNN-Enhanced.vpy"));settings.setValue("render/threeStage",true);settings.setValue("render/secondUpscale",3);}
        for(int attempt=0;attempt<2;++attempt){
            PlayerWindow player;QCOMPARE(player.settings_->value("player/animeStage").toInt(),6);
            QCOMPARE(player.settings_->value("player/animeStages").toStringList(),QStringList({"6","7","5"}));
            QCOMPARE(player.fixedAnimeStage(),6);QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),3);
        }
        PlayerWindow player;
        for(const auto &name:QStringList{"Anime-3-No-CNN.vpy","Anime-6-1X-Jinc.vpy","Anime-7-1X-D3D11.vpy"}){
            player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/"+name));QCOMPARE(player.fixedAnimeStage(),7);
            QCOMPARE(QFileInfo(player.preset_).fileName(),QString("Anime-7-AFast-1X.vpy"));QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),3);
        }
    }
    void runAnimeSixPresets(){
        QTemporaryDir directory;const auto old=qgetenv("VSR_PLAYER_CONFIG");
        qputenv("VSR_PLAYER_CONFIG",directory.filePath("player.ini").toUtf8());
        const auto restore=qScopeGuard([&]{if(old.isNull())qunsetenv("VSR_PLAYER_CONFIG");else qputenv("VSR_PLAYER_CONFIG",old);});
        PlayerWindow player;player.settings_->setValue("basic/autoplay",false);player.settings_->setValue("playback/remember",false);player.settings_->setValue("subtitle/visible",false);player.settings_->setValue("render/secondUpscale",4);
        player.resize(1280,800);player.show();player.showContextMenu(QPoint(20,20));
        auto *menu=player.findChild<QMenu *>("playerContextMenu");QVERIFY(menu);auto *anime=menu->findChild<QMenu *>("playerAnimeFilters");QVERIFY(anime);
        QCOMPARE(anime->actions().size(),6);const QList<int> stages{8,9,0,1,6,7};
        for(int i=0;i<stages.size();++i){auto *action=anime->actions()[i];QCOMPARE(action->objectName(),QString("playerAnimeStage%1").arg(stages[i]));QVERIFY(!action->toolTip().isEmpty());QVERIFY(!action->text().contains("no CNN"));}
        QVERIFY(anime->actions()[4]->toolTip().contains(QStringLiteral("第二上采样")));QVERIFY(anime->actions()[4]->toolTip()!=anime->actions()[5]->toolTip());menu->close();
        bool checked=false;
        QTimer::singleShot(100,&player,[&]{auto *dialog=player.findChild<QDialog *>("playerSettings");QVERIFY(dialog);dialog->reject();auto *list=dialog->findChild<QListWidget *>("playerAnimeStages");QVERIFY(list);QCOMPARE(list->count(),9);for(int i=0;i<list->count();++i){auto *item=list->item(i);const int stage=item->data(Qt::UserRole).toInt();QVERIFY(stage!=2 && stage!=3);if(stages.contains(stage))QVERIFY(!item->toolTip().isEmpty());}checked=true;});
        player.showSettings();QVERIFY(checked);
        QImage pattern(96,64,QImage::Format_RGB32);for(int y=0;y<pattern.height();++y)for(int x=0;x<pattern.width();++x)pattern.setPixel(x,y,qRgb((x*13+y*7)%251,(x*7+y*23)%251,(x*29+y*3)%251));QVERIFY(pattern.save(directory.filePath("input.png")));
        const auto media=directory.filePath("input.mkv");QProcess ffmpeg;ffmpeg.start(QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe"),{"-v","error","-loop","1","-i",directory.filePath("input.png"),"-t","2","-r","24","-pix_fmt","yuv444p","-c:v","ffv1","-color_primaries","bt709","-color_trc","bt709","-colorspace","bt709",media});QVERIFY(ffmpeg.waitForFinished(15000));QCOMPARE(ffmpeg.exitCode(),0);
        player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/Anime-5-D3D11.vpy"));QVERIFY(player.openFile(media));QTRY_VERIFY_WITH_TIMEOUT(player.positionRestored_ && player.ready_,15000);
        const auto files=builtinAnimeFiles();QImage enhanced;
        for(int stage:stages){
            player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/"+files[stage]));QTRY_VERIFY_WITH_TIMEOUT(player.ready_,10000);QCOMPARE(player.qualityStage_,stage);QVERIFY(player.direct_ && player.nativeAnime_);QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),4);
            qInfo()<<"six-preset"<<stage<<player.customShaderPackage_<<player.message_->text()<<player.minimalMode_<<player.imageMode_<<player.audioMode_<<player.mediaInput_;
            if(stage>=6)QTRY_VERIFY_WITH_TIMEOUT(!player.customShaderPackage_.isEmpty(),10000);else QCOMPARE(player.scalingAlgorithm(),stage==0?10:9);
            QTest::qWait(150);const auto frame=player.clock_->capture();QVERIFY(!frame.isNull());
            if(stage==6)enhanced=frame;
            if(stage==7)QVERIFY2(frame!=enhanced,"AFast 1X extra enhancement must change rendered pixels");
            if(stage==6 || stage==7){
                Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
                QVERIFY(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
                vsr::shader::Pipeline pipeline;QVERIFY(pipeline.load(player.customShaderPackage_.toUtf8().constData(),0,0,device.Get()));auto *input=pipeline.input(device.Get(),96,64);QVERIFY(input);const float color[]{.2f,.4f,.6f,1.f};context->ClearRenderTargetView(input,color);QVERIFY(pipeline.process(context.Get(),384,256));QCOMPARE(pipeline.width(),96u);QCOMPARE(pipeline.height(),64u);
                player.settings_->setValue("render/secondUpscale",0);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto nearest=player.clock_->capture();
                player.settings_->setValue("render/secondUpscale",4);player.applyScaling();player.clock_->redraw();QTest::qWait(100);const auto jinc=player.clock_->capture();QVERIFY(!nearest.isNull() && !jinc.isNull());QVERIFY2(nearest!=jinc,"Both AFast 1X variants must use the selected secondary upscaler");
            }
        }
        player.loadPreset(QDir(PresetStore::directory()).filePath("builtin/"+files[6]));player.customShaderPackage_=directory.filePath("missing.vsrhlsl");player.applyScaling();QVERIFY(player.hlslFailed_);QCOMPARE(player.qualityStage_,5);QVERIFY(player.customShaderPackage_.isEmpty());
    }

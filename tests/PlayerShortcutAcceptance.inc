void checkShortcutCatalogAndMigration() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for(bool layout120:{false,true}) {
        QSettings legacy(directory.filePath(layout120?"120.ini":"116.ini"),QSettings::IniFormat);
        const int oldDefaults=layout120?playerShortcuts(legacy).size()+3:20;
        legacy.setValue("shortcuts/layout120",layout120);
        legacy.setValue("shortcuts/rowCount",oldDefaults+1);
        legacy.setValue("shortcuts/7/key","J");legacy.setValue("shortcuts/7/action","jinc");
        legacy.setValue("shortcuts/8/key","Ctrl+K");legacy.setValue("shortcuts/8/action","d3d11");
        legacy.setValue("shortcuts/9/action","mute");
        const QString customKey=layout120 ? "Alt+H" : "Z";
        legacy.setValue(shortcutPrefix(oldDefaults)+"key",customKey);
        legacy.setValue(shortcutPrefix(oldDefaults)+"action","equalizer");
        migratePlayerShortcuts(legacy);
        const int first=playerShortcuts(legacy).size();
        QCOMPARE(legacy.value("shortcuts/rowCount").toInt(),first+3);
        QCOMPARE(legacy.value(shortcutPrefix(first)+"key").toString(),QString("Ctrl+K"));
        QCOMPARE(legacy.value(shortcutPrefix(first+1)+"key").toString(),QString("A"));
        QCOMPARE(legacy.value(shortcutPrefix(first+1)+"action").toString(),QString("mute"));
        QCOMPARE(legacy.value(shortcutPrefix(first+2)+"key").toString(),customKey);
        if(!layout120) {
            const auto defaults=playerShortcuts(legacy);
            for(int i=0;i<defaults.size();++i)
                if(defaults[i].key==customKey)QCOMPARE(legacy.value(shortcutPrefix(i)+"key",customKey).toString(),QString());
        }
        migratePlayerShortcuts(legacy);
        QCOMPARE(legacy.value("shortcuts/rowCount").toInt(),first+3);
    }
    QSettings atLimit(directory.filePath("limit.ini"),QSettings::IniFormat);
    atLimit.setValue("shortcuts/rowCount",legacyShortcutLimit);
    atLimit.setValue(shortcutPrefix(legacyShortcutLimit-1)+"action","mute");
    migratePlayerShortcuts(atLimit);
    const int migratedCount=playerShortcutCount(atLimit,playerShortcuts(atLimit).size());
    QCOMPARE(atLimit.value(shortcutPrefix(migratedCount-1)+"action").toString(),QString("mute"));
    PlayerWindow player;player.show();
    const auto input=[&](Qt::Key k,Qt::KeyboardModifiers mods=Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress,k,mods);return player.handleShortcut(&event);
    };
    for(auto k:{Qt::Key_J,Qt::Key_D,Qt::Key_A})QVERIFY(!input(k));
    bool saved=false;
    QTimer::singleShot(0,&player,[&] {
        auto *dialog=player.findChild<QDialog*>("playerSettings");QVERIFY(dialog);
        const auto close=qScopeGuard([&]{if(dialog->isVisible())dialog->reject();});
        auto *table=dialog->findChild<QTableWidget*>("playerShortcuts");QVERIFY(table);
        const int defaults=playerShortcuts(*player.settings_).size();
        QCOMPARE(table->rowCount(),defaults);
        auto *categories=dialog->findChild<QListWidget*>();
        QVERIFY(categories);
        categories->setCurrentItem(categories->findItems(QStringLiteral("快捷键"),Qt::MatchExactly).value(0));
        QVERIFY(table->isVisible());
        player.settings_->setValue("render/shaderDirectory",directory.path());
        QFile shader(directory.filePath("PersonalFilter.hlsl"));
        QVERIFY(shader.open(QIODevice::WriteOnly));shader.close();
#ifndef VSR_LITE_PLAYER
        QDir user(QDir(PresetStore::directory()).filePath("user"));
        QVERIFY(user.mkpath("."));
        QTemporaryDir presets(user.filePath("shortcut-XXXXXX"));QVERIFY(presets.isValid());
        QFile vpy(presets.filePath("PersonalPreset.vpy"));QVERIFY(vpy.open(QIODevice::WriteOnly));vpy.close();
        const auto presetId="menu:playerVpy/"+QDir(PresetStore::directory()).relativeFilePath(vpy.fileName());
#endif
        const auto editAction=[&](int row) -> QComboBox* {
            table->setCurrentCell(row,1);
            table->editItem(table->currentItem());
            for(auto *combo:table->viewport()->findChildren<QComboBox*>())
                if(combo->isVisible())return combo;
            return nullptr;
        };
        auto *combo=editAction(0);QVERIFY(combo && combo->isEditable());
        for(const auto *id:{"menu:playerOpenRenderer","menu:playerLoopRenderer","menu:playerAnimeStage9",
                            "menu:playerPrimaryScale5","menu:playerSecondScale8","menu:playerDownscale6",
                            "menu:playerSubtitleVisible","menu:playerImportCube","imageTool_rotateLeft","zoomFit",
                            "menu:playerShader/PersonalFilter.hlsl"})
            QVERIFY2(combo->findData(id)>=0,id);
#ifdef VSR_LITE_PLAYER
        QCOMPARE(combo->findData("menu:playerInterpolationD3D110"),-1);
#else
        QVERIFY(combo->findData("menu:playerInterpolationD3D110")>=0);
        QVERIFY(combo->findData("menu:playerSuper/Super-0-A4K-RIFE-A4K.vpy")>=0);
        QVERIFY(combo->findData(presetId)>=0);
        QVERIFY(vpy.remove());
#endif
        combo->showPopup();
        QTest::keyClicks(combo->lineEdit(),"sonalfil");
        auto *matches=combo->completer();
        QTRY_COMPARE(matches->completionCount(),1);
        QCOMPARE(matches->completionModel()->index(0,0).data(Qt::UserRole).toString(),QString("menu:playerShader/PersonalFilter.hlsl"));
        QTest::keyClick(matches->popup(),Qt::Key_Down);
        QTest::keyClick(matches->popup(),Qt::Key_Return);
        QTRY_COMPARE(table->item(0,1)->data(Qt::UserRole).toString(),QString("menu:playerShader/PersonalFilter.hlsl"));
        QVERIFY(shader.remove());
        const auto choose=[&](int row,const QString &id) {
            auto *editor=editAction(row);QVERIFY(editor);
            QCOMPARE(editor->findData("menu:playerShader/PersonalFilter.hlsl"),-1);
#ifndef VSR_LITE_PLAYER
            QCOMPARE(editor->findData(presetId),-1);
#endif
            const int choice=editor->findData(id);QVERIFY(choice>=0);
            editor->setCurrentIndex(choice);
            QVERIFY(QMetaObject::invokeMethod(editor,"activated",Q_ARG(int,choice)));
            QCOMPARE(table->item(row,1)->data(Qt::UserRole).toString(),id);
        };
        choose(0,"play");
        auto *add=dialog->findChild<QPushButton*>("playerAddShortcut");QVERIFY(add);
        for(const auto &[id,key]:QList<QPair<QString,QString>>{{"menu:playerSecondScale8","J"},{"menu:playerOpenRenderer","Alt+V"},
                                                              {"menu:playerLoopRenderer","Alt+B"},{"menu:playerSubtitleVisible","Alt+S"}}) {
            add->click();
            auto *recorder=table->findChild<QToolButton*>("confirmShortcutKey");QVERIFY(recorder);recorder->click();
            const int row=table->rowCount()-1;
            table->item(row,0)->setText(key);
            choose(row,id);
            QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        }
        saved=true;dialog->accept();
    });
    player.showSettings();QVERIFY(saved);
    QVERIFY(input(Qt::Key_J));QCOMPARE(player.settings_->value("render/secondUpscale").toInt(),8);
    const bool before=player.subtitleVisible_;
    QVERIFY(input(Qt::Key_S,Qt::AltModifier));QCOMPARE(player.subtitleVisible_,!before);
    QVERIFY(input(Qt::Key_V,Qt::AltModifier));QVERIFY(!player.settings_->contains("integration/renderer"));
    const auto receiver=qEnvironmentVariable("VSR_TEST_SHORTCUT_RENDERER");
    if(receiver.isEmpty())return;
    const auto oldLauncher=qgetenv("VSR_SINGLE_PLAYER_PATH");
    const auto restore=qScopeGuard([&]{if(oldLauncher.isNull())qunsetenv("VSR_SINGLE_PLAYER_PATH");else qputenv("VSR_SINGLE_PLAYER_PATH",oldLauncher);});
    qputenv("VSR_SINGLE_PLAYER_PATH",directory.filePath("Player.exe").toUtf8());
    QVERIFY(QFile::copy(receiver,directory.filePath("VSRenderer.exe")));
    const auto source=directory.filePath(QStringLiteral("片段 test.mkv"));
    QVERIFY(QFile::copy(":/startup/warmup.mkv",source));
    player.settings_->setValue("basic/autoplay",false);
    QVERIFY(player.openFile(source));
    QTRY_VERIFY_WITH_TIMEOUT(player.ready_ && player.snapshot().duration100ns>0,8000);
    const auto log=directory.filePath("received.txt");
    const auto args=[&] {QFile file(log);return file.open(QIODevice::ReadOnly)?QString::fromUtf8(file.readAll()).trimmed().split('\n'):QStringList();};
    QVERIFY(input(Qt::Key_V,Qt::AltModifier));
    QTRY_COMPARE(args(),QStringList({"--source",source}));
    player.setLoopPoint(false,10000);player.setLoopPoint(true,20000);
    QVERIFY(input(Qt::Key_B,Qt::AltModifier));
    QTRY_COMPARE(args(),QStringList({"--source",source,"--loop-in","10000","--loop-out","20000"}));
}

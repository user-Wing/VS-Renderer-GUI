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

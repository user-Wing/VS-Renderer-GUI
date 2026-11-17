    void runUpstreamBluetoothPolicy(){
        BluetoothAudio audio;audio.protocol=1;audio.connected=true;
        QCOMPARE(bluetoothCompensation(audio,1,60,0),qint64(-1616667));
        QCOMPARE(bluetoothCompensation(audio,5,60,0),qint64(-16667));
        audio.protocol=2;QCOMPARE(bluetoothCompensation(audio,5,60,0),qint64(-1366667));
        audio.protocol=3;QCOMPARE(bluetoothCompensation(audio,1,60,0),qint64(-266667));
        audio.connected=false;QCOMPARE(bluetoothCompensation(audio,1,60,0),qint64(0));
        const auto current=detectBluetoothAudio();qInfo()<<"Bluetooth endpoint"<<current.protocol<<current.connected<<current.name;
    }
    void runUpstreamRawImage(){
        const auto path=qEnvironmentVariable("VSR_RAW_TEST_SOURCE");QVERIFY2(!path.isEmpty(),"Set VSR_RAW_TEST_SOURCE to a camera RAW fixture");
        QVERIFY(PlayerImage::supports(path));QVERIFY(playerImageExtensions().contains("dng"));
        PlayerImage loader;QSignalSpy loaded(&loader,&PlayerImage::loaded),failed(&loader,&PlayerImage::failed);
        loader.open(path);QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !failed.isEmpty(),30000);
        QVERIFY2(failed.isEmpty(),failed.isEmpty()?"":qPrintable(failed.first().first().toString()));
        const auto image=qvariant_cast<QImage>(loaded.first().first());QVERIFY(!image.isNull());QCOMPARE(image.format(),QImage::Format_RGBA64);
        QVERIFY(image.text("decoder").contains("LibRaw"));QVERIFY(image.width()>32 && image.height()>32);
        int low=65535,high=0;for(int y=0;y<image.height();y+=7)for(int x=0;x<image.width();x+=7){const auto pixel=reinterpret_cast<const QRgba64 *>(image.constScanLine(y))[x];low=std::min(low,int(pixel.red()));high=std::max(high,int(pixel.red()));}QVERIFY(high-low>1000);
        QTemporaryDir bad;QFile invalid(bad.filePath("invalid.dng"));QVERIFY(invalid.open(QIODevice::WriteOnly));invalid.write("not a RAW image");invalid.close();loaded.clear();loader.open(invalid.fileName());QTRY_VERIFY_WITH_TIMEOUT(!failed.isEmpty(),10000);QVERIFY(loaded.isEmpty());
    }

#include <mainwindow.h>

#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>
#ifdef Q_OS_MAC
    #include <ApplicationServices/ApplicationServices.h>
#endif
#include <iostream>
#include <QMessageBox>
#include <QPushButton>

void debugFormatVersion()
{
    QSurfaceFormat form = QSurfaceFormat::defaultFormat();
    QSurfaceFormat::OpenGLContextProfile prof = form.profile();

    const char *profile =
        prof == QSurfaceFormat::CoreProfile ? "Core" :
        prof == QSurfaceFormat::CompatibilityProfile ? "Compatibility" :
        "None";

    printf("Requested format:\n");
    printf("  Version: %d.%d\n", form.majorVersion(), form.minorVersion());
    printf("  Profile: %s\n", profile);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set OpenGL 4.0 and, optionally, 4-sample multisampling
    QSurfaceFormat format;
    format.setVersion(4, 0);
    format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
    format.setProfile(QSurfaceFormat::CoreProfile);
    //format.setSamples(4);  // Uncomment for nice antialiasing. Not always supported.

    /*** AUTOMATIC TESTING: DO NOT MODIFY ***/
    /*** Check whether automatic testing is enabled */
    /***/ if (qgetenv("CIS277_AUTOTESTING") != nullptr) format.setSamples(0);

    QSurfaceFormat::setDefaultFormat(format);
    debugFormatVersion();

    MainWindow w;
    w.show();

#ifdef Q_OS_MAC
    bool trusted = false;
    while (!trusted) {
        CFStringRef keys[] = { kAXTrustedCheckOptionPrompt };
        CFTypeRef values[] = { kCFBooleanTrue };
        CFDictionaryRef options = CFDictionaryCreate(NULL,
                                                     (const void **)&keys,
                                                     (const void **)&values,
                                                     sizeof(keys) / sizeof(keys[0]),
                                                     &kCFTypeDictionaryKeyCallBacks,
                                                     &kCFTypeDictionaryValueCallBacks);
        if (AXIsProcessTrustedWithOptions(options)) {
            std::cout << "Trusted accessibility client!" << std::endl;
            trusted = true;
        } else {
            std::cout << "Not a trusted accessibility client." << std::endl;
            QMessageBox msgBox;
            msgBox.setWindowTitle("Allow Accessibility");
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setText("Please allow MiniMinecraft to move the mouse. You may need to go to Settings -> Privacy & Security -> Accessibility, and then find MiniMinecraft and toggle it on.<br><br><i>If it does not work, try removing MiniMinecraft from the list (with the \"-\" button) and then retrying.</i>");
            QPushButton* retryButton = new QPushButton("Retry");
            msgBox.addButton(retryButton, QMessageBox::AcceptRole);
            QPushButton* closeButton = new QPushButton("Close");
            msgBox.addButton(closeButton, QMessageBox::RejectRole);
            if (msgBox.exec() == QMessageBox::RejectRole) {
                QApplication::quit();
                return 0;
            }
        }
        CFRelease(options);
    }
#endif

    return a.exec();
}

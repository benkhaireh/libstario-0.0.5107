#include <qapplication.h>
#include "stariotest.h"

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );
    StarIOTest w;
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}

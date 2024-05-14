SOURCES	+= main.cpp
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
CONFIG	+= qt warn_on release thread
FORMS	= stariotest.ui
TEMPLATE	=app
CONFIG	+= qt warn_on release thread
LIBS	+= -lstario
LANGUAGE	= C++

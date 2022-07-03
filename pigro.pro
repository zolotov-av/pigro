TEMPLATE=subdirs


SUBDIRS+=pigro
SUBDIRS+=pigro-console
SUBDIRS+=pigro-gui


pigro-console.depends = pigro
pigro-gui.depends = pigro

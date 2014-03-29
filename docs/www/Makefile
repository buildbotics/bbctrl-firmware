all: index.html

%.html: %.jade
	jade -P <$<>$@

clean:
	rm index.html

publish: index.html
	rsync -Lav . root@openscam.org:/var/www/cbang.org/http/

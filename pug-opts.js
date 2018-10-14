{
  pretty: true,
  filters: {
    browserify: function (text, options) {
      return require('child_process').execSync(
        `./node_modules/.bin/browserify - --basedir src/js`,
        {input: text}
      ).toString()
    }
  }
}

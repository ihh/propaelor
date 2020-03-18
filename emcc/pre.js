// This code goes at the start of the generated historian JS
Module.noInitialRun = true;

Module.print = function (x) {
  if (Module.stdout && typeof(Module.stdout) === 'function')
    Module.stdout (x)
  else
    console.log (x)
}

Module.printErr = function (x) {
  if (Module.stderr && typeof(Module.stderr) === 'function')
    Module.stderr (x)
  else
    console.warn (x)
}

Module.runtimeInitPromise = new Promise ((resolve, reject) => {
  Module.onRuntimeInitialized = resolve
})

Module.runWithFiles = (args, config) => {
  config = config || {}
  return Module.runtimeInitPromise
    .then (() => {
      args = args || [];

      let nFiles = 0, filePrefix = 'FILE', outputs = []
      const wrappedArgs = args.map ((opt) => {
	if (typeof(opt) !== 'string') {
	  const filename = opt.filename || (filePrefix + (++nFiles))
          if (opt.input || opt.data) {
            const data = opt.data || ''
            const fileBuffer = new Uint8Array (data.split('').map((c)=>c.charCodeAt(0)))
	    Module.FS.writeFile (filename, fileBuffer)
          }
          if (opt.output)
            outputs.push (filename)
	  return filename
	} else
	  return opt
      })

      let stdout = config.stdout ? null : [];
      let stderr = config.stderr ? null : [];

      const oldStdout = Module.stdout
      const oldStderr = Module.stderr
      
      Module.stdout = config.stdout || ((x) => stdout.push(x));
      Module.stderr = config.stderr || ((x) => stderr.push(x));

      Module.callMain (wrappedArgs)

      Module.stdout = oldStdout
      Module.stderr = oldStderr

      let file = {}
      outputs.forEach ((filename) => {
        file[filename] = Module.FS.readFile (filename, { encoding: 'utf8'})
      })
      return { stdout, stderr, outputs, file }
    })
}

// Web worker
onmessage = function(e) {
  Module.runWithFiles (e.data, { stderr: (progress) => postMessage ({ progress }) })
    .then ((result) => {
      postMessage ({ result });
    })
}
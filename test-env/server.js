const http = require('node:http');
const fs = require('node:fs');

server = http.createServer((request, response) => {
	if (request.method != 'GET') {
		res.writeHead(405);
		res.end();
		return;
	}

	let path = new URL(
		`http://${process.env.HOST ?? 'localhost'}${request.url}`)
		.pathname;
	if (path == '/') {
		path = '/index.html';
	}
	const file = fs.readFileSync('.' + path);

	if (path.endsWith('.html')) {
		response.writeHead(200, { 'Content-Type': 'text/html' });
	} else if (path.endsWith('.wasm')) {
		response.writeHead(200,
			{ 'Content-Type': 'application/wasm' });
	} else {
		response.writeHead(200, { 'Content-Type': 'text/plain' });
	}
	response.end(file);
});

server.listen(8080, '127.0.0.1', 800);

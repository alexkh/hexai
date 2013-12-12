function Hexreport(data) {
	var my = this;
	my.data = data;
	my.match = data.match;
	my.div = $('#reportbody'); // report body div

	my.stringFill = function(x, n) {
	    var s = '';
	    for (;;) {
	        if (n & 1) s += x;
	        n >>= 1;
	        if (n) x += x;
	        else break;
	    }
	    return s;
	}

	// generate board
	my.gen_board = function(board_side, x_row, o_col, match) {
		div = match.board;
		var hh = 500; // hexagon height
		var hw = Math.sqrt(3) / 2 * hh; // hexagon width
		var xoff = 0; // x-offset
		var yoff = 20;
		var R = Raphael(div[0]);
		R.setViewBox(0, 0, 5000, 5000);
		// draw the hexagons
		for(var i = 0; i < board_side; ++i) {
			xoff += (0.5 * hw);
			for(var j = 0; j < board_side; ++j) {
				// determine center point
				var cx = Math.round(xoff + 0.5 * hw + j * hw);
				var cy = Math.round(
					yoff + 0.5 * hh + i * 0.75 * hh);
				var hexagon = R.path('M' +
					Math.round(cx - 0.5 * hw) + ',' +
					Math.round(cy - 0.25 * hh) +
					'L' + cx + ',' +
					Math.round(cy - 0.5 * hh) +
					'L' + Math.round(cx + 0.5 * hw) + ',' +
					Math.round(cy - 0.25 * hh) +
					'L' + Math.round(cx + 0.5 * hw) + ',' +
					Math.round(cy + 0.25 * hh) +
					'L' + cx + ',' +
					Math.round(cy + 0.5 * hh) +
					'L' + Math.round(cx - 0.5 * hw) + ',' +
					Math.round(cy + 0.25 * hh) +
					'L' + Math.round(cx - 0.5 * hw) + ',' +
					Math.round(cy - 0.25 * hh) + 'Z')
					.attr('fill', '#f2eac7')
					.attr('stroke', '#aaa')
					.attr('stroke-width', 1);
				if(x_row[i] & (1 << j)) {
					hexagon.attr('fill', '#000');
				} else if(o_col[j] & (1 << i)) {
					hexagon.attr('fill', '#fff');
				}
			}
		}
	}

	for(var i in my.match) {
		var match = my.match[i];
		var board_side = match.board_side;
		// create bitwise representations of board using black and white
		// array of 32-bit rows for blacks, cols for whites
		match.x_row = []; // array of 32-bit ints holding black stones
		match.o_col = []; // array of 32-bit ints holding white stones
		match.cell = []; // array will hold SVG board cells
		for(var j = 0; j < board_side; j++) {
			match.x_row[j] = 0;
			match.o_col[j] = 0;
		}
		match.board = $('<div>').addClass('board');
		match.list = $('<div>').addClass('list');
		match.lpre = $('<pre>');
		match.list.append(match.match_id + '<br />\n');
		match.list.append((match.winner == 'X'? '<b>+</b>': '<nbsp>') +
			'Black: ' + match.x_id + '<br />\n');
		match.list.append((match.winner == 'O'? '<b>+</b>': '<nbsp>') +
			'White: ' + match.o_id + '<br />\n');
		match.list.append(match.lpre);
		match.separator = $('<div>').addClass('separator');
		my.div.append(match.board);
		my.div.append(match.list);
		my.div.append(match.separator);
		match.lpre.append('#    X   O\t\ttime X\ttime O \n');
		for(var j in my.match[i].move) {
			var move = my.match[i].move[j];
			if(!(j % 2)) {
				mn = Math.floor(j / 2) + 1;
				match.lpre.append(mn + '. ' + (mn > 9?'': ' '));
			}
			match.lpre.append(move);
			if(j %2) {
				match.lpre.append('\t\t' +
					my.match[i].time_ms[j - 1]
					+ '\t' + my.match[i].time_ms[j] + '\n');
			} else {
				if(j == (my.match[i].move.length - 1)) {
					match.lpre.append('\t\t' +
					my.match[i].time_ms[j] +
					'\n');
				} else if(my.match[i].move[j].length == 3) {
					match.lpre.append(' ');
				} else {
					match.lpre.append('  ');
				}
			}
			var col = move.charCodeAt(0) - 97;
			var row = move.substring(1) - 1;
			if(j % 2) { // white's move
				match.o_col[col] |= (1 << row);
			} else {
				match.x_row[row] |= (1 << col);
			}
//			my.div.append(col).append(", " + row + '; ');
		}
		// create an image div for this match
		// now we can draw the board
		my.gen_board(board_side,match.x_row, match.o_col, match);
	}

}

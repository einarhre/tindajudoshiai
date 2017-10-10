;;
;; Common code
;;

(defun list (x . y)
  (cons x y))

;; Returns nth element of lis.
(defun nth (lis n)
  (if (= n 0)
      (car lis)
    (nth (cdr lis) (- n 1))))

;; column x positions
(define xs '(30 80 280 480 680))

(define height 60) ; height of one match
(define x0 (nth xs 0))
(define x1 (nth xs 1))
(define x2 (nth xs 2))
(define x3 (nth xs 3))
(define x4 (nth xs 4))

(define y0 170) ; y start position
(define i 0)
(define mat 1)

(define line-num 0)

(defun attr (name val)
  (write " " name "='" val "'"))

(defun write-list (arg)
  (if (atom arg)
      (if (not (null arg)) (write arg))
    (write-list (car arg))
    (write-list (cdr arg))))

;;
;; Code for next matches
;;

(defun yy (num line)
  (+ y0 10 (* (- num 1) (+ height 4)) (* line (/ height 3))))

(defun rect (x y w h color extra)
  (write "<rect") (attr "x" x) (attr "y" y)
  (attr "height" h) (attr "width" w)
  (write " " extra)
  (write " style='fill:" color ";fill-opacity:1;" "stroke:#000000;"
	 "stroke-width:1.0;" "stroke-miterlimit:4;"
	 "stroke-dasharray:none;" "stroke-opacity:1'")
  (write " />\n"))

(defun text (x y txt size weight)
  (write "<text style='font-style:normal;" "font-weight:" weight ";"
	 "font-size:" size "px;" "font-family:sans-serif;"
	 "fill:#000000;fill-opacity:1;stroke:none;'")
  (attr "x" x) (attr "y" y) (write " >") (write-list txt) (write "</text>\n"))

(defun text-r (x y txt size weight)
  (write "<text style='font-style:normal;" "font-weight:" weight ";"
	 "font-size:" size "px;" "font-family:sans-serif;"
	 "text-align:end;text-anchor:end;"
	 "fill:#000000;fill-opacity:1;stroke:none;'")
  (attr "x" x) (attr "y" y) (write " >") (write-list txt) (write "</text>\n"))

(defun num-rect (num offset)
  (rect x0 (yy num 0) 80 height "#c8c8c8" "ry='22'")
  (text-r (- x1 5) (yy num 2) (+ num offset) 24 "bold")

  (rect x1 (yy num 0) (- x2 x1) (/ height 3) "#c8c8c8" "")
  (rect x2 (yy num 0) (- x3 x2) (/ height 3) "#c8c8c8" "")
  (rect x3 (yy num 0) (- x4 x3) (/ height 3) "#c8c8c8" "")

  (rect x1 (yy num 1) (- x2 x1) (/ height 3) "#ffffff" "")
  (rect x2 (yy num 1) (- x3 x2) (/ height 3) "#ffffff" "")
  (rect x3 (yy num 1) (- x4 x3) (/ height 3) "#ffffff" "")

  (rect x1 (yy num 2) (- x2 x1) (/ height 3) "#e0ecff" "")
  (rect x2 (yy num 2) (- x3 x2) (/ height 3) "#e0ecff" "")
  (rect x3 (yy num 2) (- x4 x3) (/ height 3) "#e0ecff" ""))

(defun print-next (tatami num offset)
  (get-next-match tatami (+ num offset))
  (if (>= %number 1000) ()
    (get-data-by-ix %catix)
    (text-r (- (nth xs (+ tatami 1)) 5) (- (yy num 1) 4)
	  (list %last " kg: " (round-name %roundnum) " (" %number ")") 12 "bold")

    (if (< %compix_1 10) ()
      (get-data-by-ix %compix_1)
      (text-r (- (nth xs (+ tatami 1)) 5) (- (yy num 2) 4)
	    (list %last ", " %first " (" %country ")") 12 "normal"))

    (if (< %compix_2 10) ()
      (get-data-by-ix %compix_2)
      (text-r (- (nth xs (+ tatami 1)) 5) (- (yy num 3) 4)
	    (list %last ", " %first " (" %country ")") 12 "normal"))))

(defun print-info (r)
  (write (car r)))

(defun tournament-name ()
  (sql print-info "select value from info where item='Competition'"))

(defun tournament-date ()
  (sql print-info "select value from info where item='Date'"))

(defun write-info (what)
    (sql print-info "select value from info where item='" what "'"))

(defun draw-next (offset)
  (if (= %page 0) (setq offset 0)
    (if (= %page 1) (setq offset 12)
      (if (= %page 2) (setq offset 24)
	(setq offset 0))))
  (setq i 1)
  (while (<= i 12)
    (num-rect i offset)
    (setq mat 1)
    (while (<= mat 3)
      (print-next mat i offset)
      (setq mat (+ mat 1)))
    (setq i (+ i 1))))


;;
;; Code for medal list
;;

(defun print-comp (c x y)
  (if (< c 10) ()
    (get-data-by-ix c)
    (if (= (logand %compflags 2) 2) () ; hansokumake
      (write "<text style='font-style:normal;" "font-weight:normal;"
	     "font-size:12px;" "font-family:sans-serif;"
	     "fill:#000000;fill-opacity:1;stroke:none;'")
      (attr "x" x) (attr "y" y) (write " >" %last ", " %first ",  " %club)
      (write "</text>\n"))))

(defun print-cat-medals (line)
  ((lambda (y pline)
     (if (= %page 0) (setq pline line-num) (setq pline (- line-num 15)))
     (if (or (>= pline 15) (< pline 0)) ()
       (setq y (+ 100 (* pline 50)))
       (if (> pline 0)
	   (write "<use xlink:href='#g4151' transform='translate(0, " (- y 100) ")' />\n"))
       (text-r 95 (+ y 20) (car line) 14 "bold")
       (print-comp (nth line 2) 125 (+ y 16))
       (print-comp (nth line 3) 425 (+ y 16))
       (print-comp (nth line 4) 125 (+ y 36))
       (if (> (nth line 1) 1)
	   (print-comp (nth line 5) 425 (+ y 36)))
       )) 0 0)
  (setq line-num (+ line-num 1)))

(defun print-medals ()
  (setq line-num 0)
  (sql print-cat-medals "select category,system,pos1,pos2,pos3,pos4 from categories where pos1>0 order by category asc"))

(defun count-lines (line)
  (setq line-num (+ line-num 1)))

(defun set-medal-pages ()
  (setq line-num 0)
  (sql count-lines "select pos1 from categories where pos1 >= 10 order by category asc")
  (if (> line-num 30) (set-pages 3)
    (if (> line-num 15) (set-pages 2)
      (set-pages 1)))
  (if (= line-num 0) (write "Print results with statistics first.")))


;;
;; Code for match statistics
;;

(defun set-match-stat-pages ()
  (setq line-num 0)
  (sql count-lines "select 'index' from competitors where deleted&2=0 and weight>0")
  (set-pages (+ 1 (/ line-num 30))))

(defun print-match-stat (line)
  ((lambda (y pline)
     (setq pline (- line-num (* %page 40)))
     (if (or (>= pline 40) (< pline 0)) ()
       (setq y (+ 140 (* pline 20)))
       (text 60 y (list (nth line 1) ", " (nth line 0) ",  " (nth line 2)) 12 "normal")
       (text 400 y (nth line 3) 12 "normal")
       (text 530 y (nth line 4) 12 "normal")
       (text 575 y (nth line 5) 12 "normal")
       (text 640 y (nth line 6) 12 "normal")
       )) 0 0)
  (setq line-num (+ line-num 1)))


(defun print-match-stats (club)
  (setq line-num 0)
  (sql print-match-stat
       "select c.first, c.last, c.club, c.category,"
       " ( select count(1) from matches where"
       " (c.'index'=matches.blue and matches.blue_points>0) or"
       " (c.'index'=matches.white and matches.white_points>0)"
       " ) as 'Winnings',"
       " (select count(1) from matches where"
       " (c.'index'=matches.blue and matches.blue_points=0 and matches.white_points>0) or"
       " (c.'index'=matches.white and matches.white_points=0 and matches.blue_points>0)"
       " ) as 'Losses',"
       " (select case"
       " when pos1=c.'index' then '1st'"
       " when pos2=c.'index' then '2nd'"
       " when pos3=c.'index' then '3rd'"
       " when pos4=c.'index' and system<>1 then '3rd'"
       " when pos4=c.'index' and system=1 then '4th'"
       " when pos5=c.'index' or pos6=c.'index' then '5th'"
       " when pos7=c.'index' or pos8=c.'index' then '7th'"
       " else '-'"
       " end"
       " from categories,matches where"
       " categories.'index'=matches.category and"
       " (c.'index'=matches.blue or c.'index'=matches.white)"
       ") as 'position'"
       " from competitors as c where c.weight > 0"
       " and c.club like '" club "' "
       " group by last,first"
       )
  )


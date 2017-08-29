;; It is possible to call Lisp functions from an SVG file.
;;   %l-'m23 (pts 1)'
;;     Calls function pts with argument 1.
;;     Before calling (pts 1) the following match #23 related global variables are set:
;;     %category
;;     %number
;;     %comp_1
;;     %comp_2
;;     %score_1
;;     %score_2
;;     %points_1
;;     %points_2
;;     %match_time
;;     %comment
;;     %tatami
;;     %group
;;     %flags
;;     %forcedtatami
;;     %forcednumber
;;     %date
;;     %legend
;;     %roundnum

;;   %l-'m23 w (judoka 1)'
;;     Before calling function judoka match #23 related global variabled are set.
;;     In addition also white competitor related variables are set:
;;     %first_1
;;     %last_1
;;     %club_1
;;     %country_1
;;   Similarily, blue competitor data can be set:
;;   %l-'m23 b (judoka 2)'
;;     %first_2
;;     %last_2
;;     %club_2
;;     %country_2


;; Write points.
;;
;; Points in the database can have the following values:
;;   1:  Win by shido
;;   2:  Win in golden score when that always scores 1 point
;;   3:  Win by koka (not used)
;;   5:  Win by yuko (not used)
;;   7:  Win by waza-ari
;;   10: Win by ippon
;;   11: Hikiwake
;; Real points and how they are shown depend on the national rules.
(defun print-pts(p)
  (if (= p 0) (write "LOSS")
    (if (= p 7) (write 5)
      (if (= p 11) (write "DRAW")
	(write p)))))

;; Points analysis
(defun pts (who)				; who = 1 means white, 2 means blue
  (if (or (> %points_1 0) (> %points_2 0))	; is match finished?
      (if (= %points_1 %points_2)		; yes, match finished, but are the points equal?
	  (write "DRAW")                	; yes, points are equal, write DRAW
	(if (= who 1)				; is who = white competitor?
	    (print-pts %points_1)		; yes, write white points
	  (print-pts %points_2)))))		; no, write blue points

;; Write information about competitor.
(defun judoka (who)
  (if (= who 1)						; is who = white?
      (write %last_1 ", " %first_1 " " %country_1)	; yes, write white's name
    (write %last_2 ", " %first_2 " " %country_2)))	; no, write blue's name

(defun result ()
  (write "Name: " %first " " %last " Weight:" %weight))


;; Returns nth element of a list.
(defun nth (lis n)
  (if (= n 0)
      (car lis)
    (nth (cdr lis) (- n 1))))

;; (let1 var val body ...)
;; => ((lambda (var) body ...) val)
(defmacro let1 (var val . body)
  (cons (cons 'lambda (cons (list var) body))
	(list val)))

(define l1 0)
(define l2 0)
(defun print-match (m)
  (get-data-by-ix (nth m 2))
  (setq l1 (copy %last))
  (get-data-by-ix (nth m 3))
  (setq l2 (copy %last))
  (write l1 " vs " l2 "\n"))

(defun print-matches ()
  (get-data-by-ix %catix)
  (write "CATEGORY " %last "\n")
  (sql print-match "select * from matches where category=10014")
  )

(define grades '("?" "6.kyu" "5.kyu" "4.kyu" "3.kyu" "2.kyu" "1.kyu" "1.dan" "2.dan" "3.dan"))

(defun print-grade (grd)
  (write (nth grades grd)))

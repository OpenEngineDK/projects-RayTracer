(define funs '())

(define add-to-process (lambda (name fn)	
	(set! funs 
	(if (assoc name funs)
		(map (lambda (x) 
			(if (equal? name (car x))
				(cons name fn)
				x)
			) funs)
		(acons name fn funs)))
		(display funs)
		))
		



(define apply-process (lambda (n)
	(map (lambda (fn)
			(apply (cdr fn) n '())
		
		) funs)))
	
	

(define process (lambda (n)
	;(display n)
	;(display "\n")
		(apply-process n)
			;(test n)
	))


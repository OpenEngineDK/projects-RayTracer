(define dt 1)

(define test 
	(lambda (n)
		;(vec-to-list (tn-get-pos dot-tn))
      ;(display 'test)
		(set! dt (+ dt (/ n 5000000)))
		(let* (	(v (vec-to-list (oe-call dot-tn 'get-rotation)))
			  	(r (car v))
				(p (cadr v))
				(j (caddr v))
					
                ;(n-vec `(,(+ (* (sin dt) 2) 4)
                ;,y 
                ;,(+ (* (cos dt) 2) 4)))
                (n-vec `(,(* 2 dt)
                         ,(* 2 dt)
                          0))
                )
          (oe-call dot-tn 'set-rotation (list-to-vec n-vec))
		
          (display v)
          (display "\n")
		)))
	
			
;(add-to-process 'test test)

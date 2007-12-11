// TODO: Add licence
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL:$
// $Id: $
// 
//
// Author(s)     : Eric Berberich <eric@mpi-inf.mpg.de>
//                 Pavel Emeliyanenko <asm@mpi-sb.mpg.de>
//
// ============================================================================

/*! \file Algebraic_curve_kernel_2.h
 *  \brief defines class \c Algebraic_curve_kernel_2
 *  
 *  Curve and curve pair analysis for algebraic plane curves
 */

#ifndef CGAL_ALGEBRAIC_CURVE_KERNEL_2_H
#define CGAL_ALGEBRAIC_CURVE_KERNEL_2_H

#include <CGAL/basic.h>
#include <CGAL/Algebraic_kernel_1.h>

#include <CGAL/Algebraic_curve_kernel_2/LRU_hashed_map.h>
#include <algorithm>

#include <CGAL/Algebraic_curve_kernel_2/Xy_coordinate_2.h>
#include <CGAL/Algebraic_curve_kernel_2/Algebraic_real_traits.h>
#include <CGAL/Algebraic_curve_kernel_2/Curve_analysis_2.h>
#include <CGAL/Algebraic_curve_kernel_2/Curve_pair_analysis_2.h>

CGAL_BEGIN_NAMESPACE

template < class AlgebraicCurvePair_2, class AlgebraicKernel_1 >
class Algebraic_curve_kernel_2 {

// for each predicate functor defines a member function returning an instance
// of this predicate
#define CGAL_Algebraic_Kernel_pred(Y,Z) \
    Y Z() const { return Y(); }

// the same for construction functors
#define CGAL_Algebraic_Kernel_cons(Y,Z) CGAL_Algebraic_Kernel_pred(Y,Z)
    
private:
    //! \name wrapping types
    //!@{

    //! type of an internal curve pair
    typedef AlgebraicCurvePair_2 Internal_curve_pair_2;

    //! type of an internal curve
    typedef typename AlgebraicCurvePair_2::Algebraic_curve_2 Internal_curve_2;

    //! type of internal x_coordinate
    typedef typename Internal_curve_2::X_coordinate Internal_x_coordinate;
    
    //! type of internal coefficient
    typedef typename Internal_curve_2::Coefficient Internal_coefficient;

    //! type of internal polynomial
    typedef typename Internal_curve_2::Poly_d Internal_polynomial_2;
    
    typedef typename NiX::Polynomial_traits<Internal_polynomial_2>::
        Innermost_coefficient Innermost_coefficient;
   
    //!@}
public:
    //! \name types and functors for \c GPA_2< >
    //!@{
    
    //! type of 1D algebraic kernel 
    typedef AlgebraicKernel_1 Algebraic_kernel_1;

    //! myself
    typedef Algebraic_curve_kernel_2<AlgebraicCurvePair_2, AlgebraicKernel_1>
        Self;
    
    //! type of coefficient
    typedef Internal_coefficient Coefficient;

    //! type of curve pair
    typedef Internal_curve_pair_2 Curve_pair_2;

    //! type of single curve
    typedef Internal_curve_2 Curve_2;

    //! type of x-coordinate
    typedef Internal_x_coordinate X_coordinate_1;
        
    //! new CGAL univariate polynomial type (_CGAL postfix is temporary to
    //! avoid type clashes with \c Polynomial_2 type defined later
    typedef ::CGAL::Polynomial<Innermost_coefficient> Polynomial_1_CGAL;
    //! new CGAL bivariate polynomial type
    typedef ::CGAL::Polynomial<Polynomial_1_CGAL> Polynomial_2_CGAL;
    //! bivariate polynomial traits
    typedef ::CGAL::Polynomial_traits_d< Polynomial_2_CGAL >
        Polynomial_traits_2;
    
    //!@}
private:
    //! \name private functors
    //!@{
    
    //! temporary functor providing conversion from \c Poly_in type to
    //! \c Poly_out type, required for NumeriX <-> CGAL polynomial type
    //! conversion
    template <class Poly_2_from, class Poly_2_to>
    struct Polynomial_converter 
    {
        typedef typename Poly_2_from::NT Poly_1_from;
        typedef typename Poly_2_to::NT Poly_1_to;
        // needed for make_transform_iterator
        typedef Poly_1_to result_type;
        
        Poly_2_to operator()(const Poly_2_from& p) const
        {
            return Poly_2_to(
                ::boost::make_transform_iterator(p.begin(), *this),
                ::boost::make_transform_iterator(p.end(), *this));
        }
        Poly_1_to operator()(const Poly_1_from& p) const
        {
            return Poly_1_to(p.begin(), p.end());
        }
    };
    
    //! polynomial canonicalizer: temporarily we use NiX functors since
    //! \c Poly is NiX-type polynomial
    template <class Poly> 
    struct Poly_canonicalizer : public Unary_function< Poly, Poly >
    {
    // use Polynomial_traits_d<>::Canonicalize ?
        Poly operator()(Poly p) 
        {
            typedef CGAL::Scalar_factor_traits<Poly> Sf_traits;
            typedef typename Sf_traits::Scalar Scalar;
            typename Sf_traits::Scalar_factor scalar_factor;
            typename Sf_traits::Scalar_div scalar_div;
            Scalar g = scalar_factor(p);
            CGAL_assertion(g != Scalar(0));
            if(g != Scalar(1)) 
                scalar_div(p,g);
            if(p.lcoeff().lcoeff() < 0) 
                scalar_div(p,Scalar(-1));
            return p;        
        }
           
    };
    
    // to remove a confusion with Curve_pair_2
    typedef std::pair<Curve_2, Curve_2> Pair_of_curves_2;
    
    //! polynomial pair canonicalizer
    struct Curve_pair_canonicalizer  : 
        public Unary_function< Pair_of_curves_2, Pair_of_curves_2 >  {
        
        Pair_of_curves_2 operator()(const Pair_of_curves_2& p) const
        {
            typename Curve_2::Less_than less_than;
            if(less_than(p.second, p.first)) 
                return std::make_pair(p.second,p.first);
            return p;
        }
    };
    
    //! polynomial pair gcd creator
    template <class Poly> 
    struct Poly_pair_gcd_creator
    {
        typedef std::pair<Poly, Poly> Poly_pair;
        typedef Poly_pair argument_type;
        typedef Poly result_type;
            
        Poly operator()(const Poly_pair& p) const
        {
            return NiX::gcd(p.first, p.second);
        }
    };     
    
    struct Curve_pair_creator :
         public Unary_function< Pair_of_curves_2, Curve_pair_2 >  {
           
        Curve_pair_2 operator()(const Pair_of_curves_2& p) const 
        {
            return Curve_pair_2(p.first, p.second);
        }
    };
    
    struct Curve_pair_equal_to :
         public Unary_function<Pair_of_curves_2, bool>  {
     
         bool operator()(const Pair_of_curves_2& p1, 
            const Pair_of_curves_2& p2) const {
            return (p1.first.id() == p2.first.id() &&
                 p1.second.id() == p2.second.id());
         }
    };

    //typedef CGAL::Pair_lexicographical_less_than<Internal_polynomial_2,
      //  Internal_polynomial_2> Poly_pair_compare;
    
    //! type of curve cache
    typedef CGALi::LRU_hashed_map<Internal_polynomial_2, Curve_2,
        Poly_canonicalizer<Internal_polynomial_2>,
        CGALi::Poly_hasher_2<Internal_polynomial_2> > Curve_cache;
        
    //! type of curve pair cache 
    typedef CGALi::LRU_hashed_map<Pair_of_curves_2, Curve_pair_2,
        Curve_pair_canonicalizer, 
        CGALi::Curve_pair_hasher_2<Curve_2>, 
        Curve_pair_creator, Curve_pair_equal_to> Curve_pair_cache;
      
    //!@}
public:
    //!\name cache access functions
    //!@{
    //! access to the static curve cache
    static Curve_cache& get_curve_cache() 
    {
        static Curve_cache _m_curve_cache;
        return _m_curve_cache;
    }
    
    //! access to the static curve pair cache
    static Curve_pair_cache& get_curve_pair_cache() 
    {
        static Curve_pair_cache _m_curve_pair_cache;
        return _m_curve_pair_cache;
    }
    
    //!@}
    //! \name public functors and predicates
    //!@{
       
    //! NumeriX to CGAL polynomial type conversion
    typedef Polynomial_converter<Internal_polynomial_2, Polynomial_2_CGAL>
                NiX2CGAL_converter;
    //! CGAL to NumeriX polynomial type conversion
    typedef Polynomial_converter<Polynomial_2_CGAL, Internal_polynomial_2>
                CGAL2NiX_converter;
                
    //! \brief default constructor
    Algebraic_curve_kernel_2() //: _m_curve_cache() 
    {  }
    
    //! \brief constructs \c Curve_2 object, uses caching if appropriate
    struct Construct_curve_2 :
            public Unary_function< Internal_polynomial_2, Curve_2 >
    {
        //! \brief constructs an object from \c Algebraic_curve_kernel_2 type
        //! no default constructor provided
        Construct_curve_2(/*Self *pkernel_2*/) 
        {  }
            
        Curve_2 operator()(const Internal_polynomial_2& f) const
        {
            return Self::get_curve_cache()(f);
            //return Curve_2(f);
        }
        Curve_2 operator()(const Polynomial_2_CGAL& f) const
        {
            CGAL2NiX_converter cvt;
            return Self::get_curve_cache()(cvt(f));
        }
    };
    CGAL_Algebraic_Kernel_cons(Construct_curve_2, construct_curve_2_object);
    
    //! type of a curve point 
    typedef CGALi::Xy_coordinate_2<Self> Xy_coordinate_2;
    
    //! traits class for \c X_coordinate
    typedef CGALi::Algebraic_real_traits<typename Curve_2::X_coordinate>
        X_real_traits_1;

    //! traits class for \c Xy_coorinate_2
    typedef CGALi::Algebraic_real_traits<Xy_coordinate_2> Y_real_traits_1;
    
    //! returns the first coordinate of \c Xy_coordinate_2
    struct Get_x_2 :
        public Unary_function<Xy_coordinate_2, X_coordinate_1> {
        
        X_coordinate_1 operator()(const Xy_coordinate_2& xy) const {
            return xy.x();
        }
    };
    CGAL_Algebraic_Kernel_cons(Get_x_2, Get_x_2_object);
    
    //! returns the second coordinate of \c Xy_coordinate_2
    struct Get_y_2 :
        public Unary_function<Xy_coordinate_2, X_coordinate_1> {
        
        X_coordinate_1 operator()(const Xy_coordinate_2& xy) const {
            return xy.y();
        }
    };
    CGAL_Algebraic_Kernel_cons(Get_y_2, Get_y_2_object);
    
    struct Refine_x_2 :
        public Unary_function<Xy_coordinate_2, void> {
      
        //! \brief returns at least half's the current interval of the first
        //! coordinate of \c r
        //!
        //! note that an interval may also degenerate to a single point
        void operator()(const Xy_coordinate_2& r) const {
            r.refine_x();            
        }
        
        //! \brief refines the current interval of the first coordinate of \c r
        //! w.r.t. given relative precision
        //!
        //! that is:
        //! <tt>|lower - upper|/|r.x()| <= 2^(-rel_prec)</tt> 
        void operator()(Xy_coordinate_2& r, int rel_prec) const {  
            r.refine_x(rel_prec);
        }
    };
    CGAL_Algebraic_Kernel_pred(Refine_x_2, refine_x_2_object);
    
    struct Refine_y_2 :
        public Unary_function<Xy_coordinate_2, void> {
      
        //! \brief returns at least half's the current interval of the second
        //! coordinate of \c r
        //!
        //! note that an interval may also degenerate to a single point
        void operator()(const Xy_coordinate_2& r) const {
            typename Y_real_traits_1::Refine()(r);
        }
        
        //! \brief refines the current interval of the second coordinate of 
        //! \c r w.r.t. given relative precision
        //!
        //! that is:
        //! <tt>|lower - upper|/|r.y()| <= 2^(-rel_prec)</tt> 
        void operator()(Xy_coordinate_2& r, int rel_prec) const {  
            typename Y_real_traits_1::Refine()(r, rel_prec);
        }
    };
    CGAL_Algebraic_Kernel_pred(Refine_y_2, refine_y_2_object);
    
    //! computes the current lower boundary of the first coordinate of \c r
    struct Lower_boundary_x_2 {
       
        typedef Xy_coordinate_2 agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r) {
            return typename X_real_traits_1::Lower_boundary()(r.x());
        }
    };
    CGAL_Algebraic_Kernel_cons(Lower_boundary_x_2, lower_boundary_x_2_object);
    
    //! computes the current upper boundary of the first coordinate of \c r
    struct Upper_boundary_x_2 {
       
        typedef Xy_coordinate_2 agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r) {
            return typename X_real_traits_1::Upper_boundary()(r.x());
        }
    };
    CGAL_Algebraic_Kernel_cons(Upper_boundary_x_2, upper_boundary_x_2_object);
    
    //! computes the current lower boundary of the second coordinate of \c r
    struct Lower_boundary_y_2 {
       
        typedef Xy_coordinate_2 agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r) {
            return typename Y_real_traits_1::Lower_boundary()(r);
        }
    };
    CGAL_Algebraic_Kernel_cons(Lower_boundary_y_2, lower_boundary_y_2_object);
    
    //! computes the current lower boundary of the second coordinate of \c r
    struct Upper_boundary_y_2 {
       
        typedef Xy_coordinate_2 agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r) {
            return typename Y_real_traits_1::Upper_boundary()(r);
        }
    };
    CGAL_Algebraic_Kernel_cons(Upper_boundary_y_2, upper_boundary_y_2_object);
    
    //! returns the number of boundary type in-between x-coordinates of two
    //! Xy_coordinate_2 objects
    struct Boundary_between_x_2 {
       
        typedef Xy_coordinate_2 first_agrument_type;
        typedef Xy_coordinate_2 second_agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r1, 
                const Xy_coordinate_2& r2) const {
            return typename X_real_traits_1::Boundary_between()
                (r1.x(), r2.x());
        }
    };
    CGAL_Algebraic_Kernel_cons(Boundary_between_x_2, 
            boundary_between_x_2_object);
            
    //! returns the number of boundary type in-between y-coordinates of two
    //! Xy_coordinate_2 objects
    struct Boundary_between_y_2 {
       
        typedef Xy_coordinate_2 first_agrument_type;
        typedef Xy_coordinate_2 second_agrument_type;
        typedef typename Algebraic_kernel_1::Boundary result_type;
            
        result_type operator()(const Xy_coordinate_2& r1, 
                const Xy_coordinate_2& r2) const {
            return typename Y_real_traits_1::Boundary_between()(r1, r2);
        }
    };
    CGAL_Algebraic_Kernel_cons(Boundary_between_y_2, 
            boundary_between_y_2_object);
    
    //! \brief comparison of x-coordinates 
    struct Compare_x_2 :
         public Binary_function<X_coordinate_1, X_coordinate_1, 
                Comparison_result > {

        Comparison_result operator()(const X_coordinate_1& x1, 
                                         const X_coordinate_1& x2) const {
        // not yet implemented in Algebraic_kernel_1 (will it be ?)
        //   Algebraic_kernel_1 ak;
        //   return (ak.compare_x_2_object()(x1, x2));
            return x1.compare(x2);
        }
        Comparison_result operator()(const Xy_coordinate_2& xy1, 
                                         const Xy_coordinate_2& xy2) const {
            return ((*this)(xy1.x(), xy2.x()));
        }
    };
    CGAL_Algebraic_Kernel_pred(Compare_x_2, compare_x_2_object);

    //! \brief comparison of y-coordinates of two points
    struct Compare_y_2 :
        public Binary_function< Xy_coordinate_2, Xy_coordinate_2, 
                Comparison_result > {
        
        Comparison_result operator()(const Xy_coordinate_2& xy1, 
                                     const Xy_coordinate_2& xy2) const {
            
            return (Compare_x_2()(xy1.y(), xy2.y()));
        }
    };
    CGAL_Algebraic_Kernel_pred(Compare_y_2, compare_y_2_object);
    
    //! lexicographical comparison of two objects of type \c Xy_coordinate_2
    //!
    //! \c equal_x specifies that only y-coordinates need to be compared
    struct Compare_xy_2 :
          public Binary_function<Xy_coordinate_2, Xy_coordinate_2, 
                Comparison_result > 
    {
        Comparison_result operator()(const Xy_coordinate_2& xy1, 
             const Xy_coordinate_2& xy2, bool equal_x = false) const {
             return xy1.compare_xy(xy2, equal_x);
        }
    };
    CGAL_Algebraic_Kernel_pred(Compare_xy_2, compare_xy_2_object);

    //! \brief checks whether curve has only finitely many self-intersection
    //! points, i.e., it has no self-overlapped continuous parts
    //!
    //! for algerbaic curves this means that supporting polynomial is 
    //! square-free
    struct Has_finite_number_of_self_intersections_2 :
            public Unary_function< Polynomial_2_CGAL, bool > {

        bool operator()(const Polynomial_2_CGAL& p) const {

            //typename Polynomial_traits_2::Is_square_free is_square_free;
            CGAL_error_msg("is_square_free is not yet supported\n");
            return true; //is_square_free(p);
        }

        bool operator()(const Internal_polynomial_2& p) const {
            NiX2CGAL_converter cvt;
            return (*this)(cvt(p));
        }
    };
    CGAL_Algebraic_Kernel_pred(Has_finite_number_of_self_intersections_2, 
            has_finite_number_of_self_intersections_2_object);
            
    //! \brief checks whether a curve pair has finitely many intersections,
    //! in other words, whether two curves have no continuous common part
    //!
    //! in case of algerbaic curves: checks whether supporting polynomials are
    //! coprime
    struct Has_finite_number_of_intersections_2 :
            public Binary_function< Curve_2, Curve_2, bool > { 
               
        bool operator()(const Curve_2& c1, const Curve_2& c2) const {
            // if curve ids are the same - non-decomposable
            if(c1.id() == c2.id()) 
                return true;
            typename Polynomial_traits_2::Gcd_up_to_constant_factor gcd_utcf;
            typename Polynomial_traits_2::Total_degree total_degree;
            NiX2CGAL_converter cvt;
            Polynomial_2_CGAL p1 = cvt(c1.f()), p2 = cvt(c2.f());
            return (total_degree(gcd_utcf(p1, p2)) == 0);  
        }
    };
    CGAL_Algebraic_Kernel_pred(Has_finite_number_of_intersections_2, 
            has_finite_number_of_intersections_2_object);
    
    //! set of various curve and curve pair decomposition functions
    struct Decompose_2 {
    
        //! default constructor
        Decompose_2(/*Self *pkernel_2*/)  
        {  }

        //! \brief returns a curve without self-overlapping parts 
        //!
        //! in case of algebraic curves computes square-free part of supporting
        //! polynomial
        Polynomial_2_CGAL operator()(const Polynomial_2_CGAL& p) {
            typename Polynomial_traits_2::Make_square_free msf;
            return msf(p);
        }
        
        //! temporary version for \c NiX::Polynomial
        Internal_polynomial_2 operator()(const Internal_polynomial_2& p) {
            NiX2CGAL_converter cvt;
            CGAL2NiX_converter cvt_back;
            return cvt_back((*this)(cvt(p)));
        }
        
        //! \brief computes a square-free factorization of a curve \c c, 
        //! returns the number of pairwise coprime square-free factors
        //! 
        //! returns square-free pairwise coprime factors in \c fit and
        //! multiplicities in \c mit. Template argument type of \c fit is
        //! \c Curve_2, and \c mit is \c int
        template< class OutputIterator1, class OutputIterator2 >
        int operator()( const Curve_2& c, OutputIterator1 fit, 
                        OutputIterator2 mit ) const {
                        
            typename Polynomial_traits_2::
                Square_free_factorization_up_to_constant_factor factorize;
            NiX2CGAL_converter cvt;
            std::vector<Polynomial_2_CGAL> factors;
            
            int n_factors = factorize(cvt(c.f()), std::back_inserter(factors),
                    mit); 
            Construct_curve_2 cc_2;
            for(int i = 0; i < (int)factors.size(); i++) {
                *fit++ = cc_2(factors[i]);
            }
            return n_factors;
        }
        
        //! \brief computes for a given pair of curves \c c1 and \c c2 their 
        //! common part \c oib and coprime parts \c oi1 and \c oi2 
        //! respectively; returns \c true if the curves were decomposed
        //!
        //! returns true if \c c1 and \c c2 are coprime. Template argument
        //! type of \c oi{1,2,b} is \c Curve_2
        template < class OutputIterator > 
        bool operator()(const Curve_2& c1, const Curve_2& c2,
            OutputIterator oi1, OutputIterator oi2, OutputIterator oib) {
            
            typedef std::vector<Curve_2> Curves;
            Curves parts_f, parts_g;
            if(Curve_2::decompose(c1, c2, 
                std::back_inserter(parts_f), std::back_inserter(parts_g))) {
                // move the common part returned through both iterators
                // oi1/oi2 to oib
                *oib++ = parts_f[0];
                CGAL_precondition(parts_f[0].f() == parts_g[0].f());
                if(parts_f.size() > 1)
                    std::copy(parts_f.begin() + 1, parts_f.end(), oi1);
                if(parts_g.size() > 1)
                    std::copy(parts_g.begin() + 1, parts_g.end(), oi2);
                return true;
            }
            // copy original curves to the output iterator:
            *oi1++ = c1;
            *oi2++ = c2;
            return false;
        }
    private:
        //! pointer to Algebraic_curve_kernel_2 (for caching issues)
        /*Self *_m_pkernel_2; */
    };
    CGAL_Algebraic_Kernel_cons(Decompose_2, decompose_2_object);
    
    //!@}
public:
    //! \name types and functors for \c GPA_2<Algebraic_kernel_2>
    //!@{
    
    typedef Curve_2 Polynomial_2; 
    
    typedef Construct_curve_2 Construct_polynomial_2_;

    typedef X_coordinate_1 Algebraic_real_1;
    typedef Xy_coordinate_2 Algebraic_real_2;
    
    typedef Has_finite_number_of_self_intersections_2 Is_square_free_2;
    typedef Has_finite_number_of_intersections_2 Is_coprime_2;

    typedef Decompose_2 Make_square_free_2;
    typedef Decompose_2 Square_free_factorization;
    typedef Decompose_2 Make_coprime_2;
    
    //! \brief computes the derivative w.r.t. the first (innermost) variable
    struct Derivative_x_2 : 
        public Unary_function< Polynomial_2_CGAL, Polynomial_2_CGAL > {
        
        Polynomial_2_CGAL operator()(const Polynomial_2_CGAL& p) const
        {
            typename Polynomial_traits_2::Derivative derivate;
            return derivate(p, 0);
        }
    };
    CGAL_Algebraic_Kernel_cons(Derivative_x_2, derivative_x_2_object);

    //! \brief computes the derivative w.r.t. the first (outermost) variable
    struct Derivative_y_2 :
        public Unary_function< Polynomial_2_CGAL, Polynomial_2_CGAL > {
        
        Polynomial_2_CGAL operator()(const Polynomial_2_CGAL& p) const
        {
            typename Polynomial_traits_2::Derivative derivate;
            return derivate(p, 1);
        }
    };
    CGAL_Algebraic_Kernel_cons(Derivative_y_2, derivative_y_2_object);

    struct X_critical_points_2 : 
        public Binary_function< Polynomial_2, 
            std::iterator<output_iterator_tag, Xy_coordinate_2>,
            std::iterator<output_iterator_tag, Xy_coordinate_2> > {
       
        //! \brief copies in the output iterator the x-critical points of
        //! polynomial \c p as objects of type \c Xy_coordinate_2
        //!
        //! all points (x, y) with f(x,y) = fy(x,y) = 0 are x-critical points
        //! (i.e, singularities are also counted)
        template <class OutputIterator>
        OutputIterator operator()(const Polynomial_2& p, 
                OutputIterator oi) const {
                
            typename Self::Derivative_x_2 der_x;
            typename Self::Construct_curve_2 curve_2;
            NiX2CGAL_converter cvt;
            // construct curve analysis of a derivative in y
            typename Self::Curve_analysis_2 ca_2(p),
                    ca_2x(curve_2(der_x(cvt(p.f()))));
            typename Self::Curve_pair_analysis_2 cpa_2(ca_2, ca_2x);
            typename Self::Curve_pair_analysis_2::Status_line_1 cpv_line;
            typename Self::Curve_analysis_2::Status_line_1 cv_line;
            
            int i, j, n_arcs, n_events =
                cpa_2.number_of_status_lines_with_event();
            std::pair<int,int> ipair;
            bool vline_constructed = false;
            
            for(i = 0; i < n_events; i++) {
                cpv_line = cpa_2.status_line_at_event(i);
                // no 2-curve intersections over this status line
                if(!cpv_line.is_intersection())
                    continue;
                n_arcs = cpv_line.number_of_events();
                for(j = 0; j < n_arcs; j++) {
                    ipair = cpv_line.curves_at_event(j);
                    if(ipair.first == -1||ipair.second == -1) 
                        continue;
                    if(!vline_constructed) {
                        cv_line = ca_2.status_line_at_exact_x(cpv_line.x());
                        vline_constructed = true;
                    }
                    // ipair.first is an arcno over status line of the
                    // curve p
                    *oi++ = cv_line.algebraic_real_2(ipair.first);
                }
                vline_constructed = false;
            }
            return oi;
        }
        
        //! \brief computes the ith x-critical point of polynomial \c p
        Xy_coordinate_2 operator()(const Polynomial_2& p, int i) const
        {
            std::vector<Xy_coordinate_2> x_points;
            (*this)(p, std::back_inserter(x_points));
            CGAL_precondition(0 >= i&&i < x_points.size());
            return x_points[i];
        }
    };
    CGAL_Algebraic_Kernel_cons(X_critical_points_2,
        x_critical_points_2_object);
    
    struct Y_critical_points_2 :
        public Binary_function< Polynomial_2, 
            std::iterator<output_iterator_tag, Xy_coordinate_2>,
            std::iterator<output_iterator_tag, Xy_coordinate_2> > {
    
        //! \brief copies in the output iterator the y-critical points of
        //! polynomial \c p as objects of type \c Xy_coordinate_2
        template <class OutputIterator>
        OutputIterator operator()(const Polynomial_2& p, 
            OutputIterator oi) const
        {
            typedef typename Self::Curve_analysis_2 Curve_analysis_2;
            typedef typename Self::Curve_pair_analysis_2 Curve_pair_analysis_2;
            
            typename Curve_analysis_2::Status_line_1 cv_line;
            std::pair<int,int> ipair;
            // p is of type Curve_2 here
            Curve_analysis_2 ca_2(p); 
            int i, j, k, n_arcs, n_events =
                ca_2.number_of_status_lines_with_event();
            
            bool cpa_constructed = false, vline_constructed = false; 
            typename Curve_pair_analysis_2::Status_line_1
                cpv_line;
            Curve_pair_analysis_2 cpa_2;
            
            for(i = 0; i < n_events; i++) {
                cv_line = ca_2.status_line_at_event(i);
                n_arcs = cv_line.number_of_events();
                for(j = 0; j < n_arcs; j++) {
                    ipair = cv_line.number_of_incident_branches(j);
                    // general case: no special tests required
                    if(!(ipair.first == 1&&ipair.second == 1)) {
                        *oi++ = cv_line.algebraic_real_2(j);
                        continue;
                    }
                    if(!cpa_constructed) {
                        typename Self::Derivative_y_2 der_y;
                        typename Self::Construct_curve_2 curve_2;
                        NiX2CGAL_converter cvt;
                        // construct curve analysis of a derivative in x
                        Curve_analysis_2 ca_2y(curve_2(der_y(cvt(p.f()))));
                        cpa_2 = Curve_pair_analysis_2(ca_2, ca_2y);
                        cpa_constructed = true;
                    }
                    if(!vline_constructed) {
                        cpv_line = cpa_2.status_line_for_x(cv_line.x());
                        vline_constructed = true;
                    }
                    if(!cpv_line.is_intersection())
                        continue;
                    // obtain the y-position of j-th event of curve p
                    k = cpv_line.event_of_curve(j, 0);
                    ipair = cpv_line.curves_at_event(k);
                    
                    // pick up only event comprised of both curve and its der
                    if(ipair.first != -1&&ipair.second != -1)
                        *oi++ = cv_line.algebraic_real_2(j);
                }
                vline_constructed = false;
            }
            return oi;
        }
        
        //! \brief computes the ith y-critical point of polynomial \c p
        Xy_coordinate_2 operator()(const Polynomial_2& p, int i) const
        {
            std::vector<Xy_coordinate_2> y_points;
            (*this)(p, std::back_inserter(y_points));
            CGAL_precondition(0 >= i&&i < y_points.size());
            return y_points[i];
        }
    };
    CGAL_Algebraic_Kernel_cons(Y_critical_points_2,
        y_critical_points_2_object);

    /*!\brief 
     * computes the sign of a bivariate polynomial \c p evaluated at the root 
     * \c r of a system of two bivariate polynomial equations
     *
     * returns a value convertible to \c CGAL::Sign
     */
    struct Sign_at_2 :
        public Binary_function< Polynomial_2, Xy_coordinate_2, Sign > {

        typedef typename Self::Y_real_traits_1 Y_real_traits_1;
        typedef typename Y_real_traits_1::Boundary Boundary;
        
        typedef boost::numeric::interval<Boundary> Interval;
        
        typedef CGAL::Polynomial<Boundary> Poly_rat_1;
        typedef CGAL::Polynomial<Poly_rat_1> Poly_rat_2;
        
        Sign operator()(const Polynomial_2& p, const Xy_coordinate_2& r) const
        {
            if(p.id() == r.curve().id()) // point lies on the same curve
                return CGAL::ZERO;
        
            NiX2CGAL_converter cvt;
            typedef typename Algebraic_kernel_1::Algebraic_real_traits
                X_real_traits_1;
            // convert poly to rational rep
            typedef CGAL::Fraction_traits<Poly_rat_2> FTraits;
            // divide by maximal coefficient ?
            typename FTraits::Denominator_type det(1);
            Poly_rat_2 rat_p = typename FTraits::Compose()(cvt(p.f()), det);

            /*typename FTraits::Numerator_type num;
             typename FTraits::Decompose()(rat_p, num, det);*/
            
            typename Y_real_traits_1::Lower_boundary lower_2;
            typename Y_real_traits_1::Upper_boundary upper_2;
            typename Y_real_traits_1::Refine refine_2;
            
            X_coordinate_1 x = r.x();
            CGAL::Sign s_lower;
            bool zero_tested = false;
            Boundary eps = Boundary(1)/Boundary(10000);
            
            while(1) {
                Interval iv, ix(x.low(), x.high()), iy(lower_2(r), upper_2(r));
                iv = _evaluate_2(rat_p, ix, iy);
                s_lower = CGAL::sign(iv.lower());
                if(s_lower == sign(iv.upper()))
                    return s_lower;

                Boundary x_len = ix.upper() - ix.lower(),
                         y_len = iy.upper() - iy.lower();

                if(!zero_tested) {
                    if(x_len < eps||y_len < eps) {
                        if(_test_exact_zero(p, r))
                            return CGAL::ZERO;
                        zero_tested = true;
                    }
                }
                // keep x/y-intervals comparable in size
                (x_len > y_len) ? x.refine() : refine_2(r);
            }
        }
        
    private:

        bool _test_exact_zero(const Polynomial_2& p,
            const Xy_coordinate_2& r) const {

            typedef typename Self::Curve_analysis_2 Curve_analysis_2;
            typedef typename Self::Curve_pair_analysis_2 Curve_pair_analysis_2;
            
            Curve_analysis_2 ca_2(p);
            typename Curve_analysis_2::Status_line_1
                cv_line = ca_2.status_line_for_x(r.x());
            // fast check for the presence of status line at r.x()
            if(cv_line.covers_line())    
                return true;

            Curve_pair_analysis_2 cpa_2(ca_2, Curve_analysis_2(r.curve()));
            typename Curve_pair_analysis_2::Status_line_1
                cpv_line = cpa_2.status_line_for_x(r.x());
            
            if(cpv_line.is_event() && cpv_line.is_intersection()) {
                // get an y-position of the point r
                int idx = cpv_line.event_of_curve(r.arcno(), 1);
                std::pair<int, int> ipair =
                      cpv_line.curves_at_event(idx);
                if(ipair.first != -1&&ipair.second != -1)
                    return true;
            }
            return false;
        }
    
        Interval _evaluate_2(const Poly_rat_2& p, const Interval& ix,
            const Interval& iy) const {

            // CGAL::Polynomial does not provide Coercion_traits for number
            // types => therefore evaluate manually
            typename Poly_rat_2::const_iterator it = p.end() - 1;
            Interval res(_evaluate_1(*it, ix));

            while((it--) != p.begin()) 
                res = res * iy + (_evaluate_1(*it, ix));
            return res;
        }

        Interval _evaluate_1(const Poly_rat_1& p, const Interval& ix) const {

            typename Poly_rat_1::const_iterator it = p.end() - 1;
            Interval res(*it);
            while((it--) != p.begin()) 
                res = res * ix + *it;
            return res;
        }
    };

    struct Sign_at_2_buggy :
        public Binary_function< Polynomial_2, Xy_coordinate_2, Sign > {
        
        Sign operator()(const Polynomial_2& p, const Xy_coordinate_2& r) const
        {
            if(p.id() == r.curve().id()) // point lies on the same curve
                return CGAL::ZERO;
            
            typedef typename Self::Curve_analysis_2 Curve_analysis_2;
            typedef typename Self::Curve_pair_analysis_2 Curve_pair_analysis_2;
            
            Curve_analysis_2 ca_2(p), ca_2r(r.curve());
            Curve_pair_analysis_2 cpa_2(ca_2, ca_2r);
            typename Curve_analysis_2::Status_line_1
                cv_line = ca_2.status_line_for_x(r.x()),
                cv_line_r = ca_2r.status_line_for_x(r.x());
            
            // fast check for the presence of vertical line at r.x()
            if(cv_line.covers_line())    
                return CGAL::ZERO;
                
            // in case there is no event at this x-coordinate, status
            // line at some rational x over an interval is returned
            typename Curve_pair_analysis_2::Status_line_1
                cpv_line = cpa_2.status_line_for_x(r.x());
                        
            // get an y-position of the point r
            int idx = cpv_line.event_of_curve(r.arcno(), 1);
            std::pair<int, int> ipair;
            X_coordinate_1 boundary_x;
            
            if(cpv_line.is_event()) {
                if(cpv_line.is_intersection()) {
                    ipair = cpv_line.curves_at_event(idx);
                    // easy case: there is a 2-curve intersection at this x
                    if(ipair.first != -1&&ipair.second != -1)
                        return CGAL::ZERO; // intersection of both curves
                }
                // check if there is an event of curve p at r.x()
                if(cv_line.is_event()) {
                    if(cv_line_r.is_event())
                        CGAL_error_msg("you're lucky )) this is not an easy \
                            case..");
                    //std::cout << "sign at event of curve p\n";
                    // this is an event of curve p but not of r.curve() ->
                    // shift to the left of r.x() otherwise we would find
                    // arc-numbers of p at event point (r.arcno() is valid
                    // over curve-pair interval)
                    cpv_line = cpa_2.status_line_of_interval(
                        cpv_line.index());
                    // recompute vertical line of p and y-position of r
                    // (however y-position of r.arcno() should not change
                    // since it can only happen at 2-curve event or at event
                    // of g)
                    idx = cpv_line.event_of_curve(r.arcno(), 1);
                    //  need cv-line over interval ?
                    cv_line = ca_2.status_line_of_interval(
                        cv_line.index());
                    boundary_x = cpv_line.x();
                    
                } else if(cv_line_r.is_event()) {
                
                   // std::cout << "sign at event of curve r: cpv_line x: "
                     //   << cpv_line.x() << "\n";
                // this is an event of r.curve() -> therefore cpv_line.x()
                // is given as algebraic real (not rational) and r.arcno() is
                // an event arcno 
                    // need to recompute boundary_x (but leave cpv_line
                    // unchanged otherwise r.arcno() is not valid)
                    boundary_x = cpa_2.status_line_of_interval(
                        cpv_line.index()).x();
                }
            } else {
            // there is no event at r.x() of curve p hence we're free to
            // pick up any rational boundary over an interval to compute the
            // sign at 
                boundary_x = cpv_line.x();
               // std::cout << "sign over curve pair interval\n";
            }
            // obtain status line at exact rational x
            cv_line = ca_2.status_line_at_exact_x(
                X_coordinate_1(boundary_x.low()));
            
            int arcno_low = -1, arcno_high = -1, i = idx;
            typedef typename Self::Algebraic_real_traits Traits;
            typedef typename Traits::Boundary Boundary;
            Boundary boundary_y;
            Xy_coordinate_2 xy1, xy2;
            
            // arcno_low and arcno_high are consecutive event indices of
            // curve p at r.x()
            while(i-- > 0) {
                ipair = cpv_line.curves_at_event(i);
                if(ipair.first != -1) {
                    arcno_low = ipair.first;
                    xy1 = cv_line.algebraic_real_2(arcno_low);
                    break;
                }
            }
            i = idx;
            while(i++ < cpv_line.number_of_events() - 1) {
                ipair = cpv_line.curves_at_event(i);
                if(ipair.first != -1) {
                    arcno_high = ipair.first;
                    xy2 = cv_line.algebraic_real_2(arcno_high);
                    break;
                }
            }
                   
            if(arcno_low != -1) {
                boundary_y = (arcno_high != -1 ? 
                     typename Traits::Boundary_between()(xy1, xy2) :
                     typename Traits::Upper_boundary()(xy1));
            } else {
                // if arcno_high == -1 pick up arbitrary rational since the
                // curve p does not cross vertical line at r.x()
                boundary_y = (arcno_high != -1 ? 
                     typename Traits::Lower_boundary()(xy2) : Boundary(0)); 
            }
            if(boundary_x.low() != boundary_x.high())
                std::cout << "oops very bizarre error occurred..\n";
              
            NiX::Polynomial<Boundary> poly = 
                NiX::substitute_x(p.f(), boundary_x.low());
                            
            return poly.sign_at(boundary_y);
        }
    };
    CGAL_Algebraic_Kernel_pred(Sign_at_2, sign_at_2_object);

    /*!\brief
     * copies in the output iterator \c roots the common roots of polynomials
     * \c p1 and \c p2 and copies in the output iterator \c mults their 
     * respective multiplicity as intergers, in the same order
     *
     * template argument type of \c roots is \c Xy_coordinate_2 , returns the
     * pair of respective past-the-end iterators
     *
     * \pre p1 and p2 are square-free and the set of solutions of the system
     * is 0-dimensional
     */  
    struct Solve_2 {
    
        typedef Polynomial_2 first_argument_type;
        typedef Polynomial_2 second_argument_type;
        typedef std::iterator<output_iterator_tag, Xy_coordinate_2>
            third_argument_type;
        typedef std::iterator<output_iterator_tag, int>
            fourth_argument_type;
        typedef std::pair<third_argument_type, fourth_argument_type>
            result_type;
     
        template <class OutputIteratorRoots, class OutputIteratorMult>
        std::pair<OutputIteratorRoots, OutputIteratorMult>
            operator()(const Polynomial_2& p1, const Polynomial_2& p2,
                OutputIteratorRoots roots, OutputIteratorMult mults) const
        {
            // these tests are quite expensive... do we really need them ??
            /*
            CGAL_precondition_code (
                typename Self::Has_finite_number_of_self_intersections_2 
                    not_self_overlapped;
                typename Self::Has_finite_number_of_intersections_2 
                    do_not_overlap;
                CGAL_precondition(not_self_overlapped(p1) &&
                    not_self_overlapped(p2));
                CGAL_precondition(do_not_overlap(p1, p2));
            );
            */
            typename Self::Curve_pair_analysis_2 cpa_2(
                (Curve_analysis_2(p1)),(Curve_analysis_2(p2)));
            typename Self::Curve_pair_analysis_2::Status_line_1
                cpv_line;
            // do we need to check which supporting curve is simpler ?    
            typename Polynomial_traits_2::Total_degree total_degree;
            NiX2CGAL_converter cvt;
            Polynomial_2_CGAL f1 = cvt(p1.f()), f2 = cvt(p2.f());
            bool first_curve = (total_degree(f1) < total_degree(f2));
            
            int i, j, n = cpa_2.number_of_status_lines_with_event();
            std::pair<int, int> ipair;
            for(i = 0; i < n; i++) {
                cpv_line = cpa_2.status_line_at_event(i);
                if(!cpv_line.is_intersection())
                    continue;
                // store x-coord for future use
                X_coordinate_1 x = cpv_line.x(); 
                for(j = 0; j < cpv_line.number_of_events(); j++) {
                    ipair = cpv_line.curves_at_event(j);
                    if(ipair.first == -1 || ipair.second == -1) 
                        continue;
                    // VOILA!! we've got it !!!
                    *roots++ = Xy_coordinate_2(x, (first_curve ? p1 : p2), 
                            (first_curve ? ipair.first: ipair.second));
                    *mults++ = cpv_line.multiplicity_of_intersection(j);
                }
            }
            return std::make_pair(roots, mults);
        }
    };
    CGAL_Algebraic_Kernel_cons(Solve_2, solve_2_object);

#undef CGAL_Algebraic_Kernel_pred    
#undef CGAL_Algebraic_Kernel_cons 
    
    //!@}
public:
    //! \name types and functors for \c GPA_2< both >
    //!@{
   
    //! type of 1-curve analysis
    typedef CGALi::Curve_analysis_2<Self> Curve_analysis_2; 

    //! type of 2-curve analysis
    typedef CGALi::Curve_pair_analysis_2<Self> Curve_pair_analysis_2; 
    
    //!@}
      
}; // class Algebraic_curve_kernel_2

CGAL_END_NAMESPACE

#endif // CGAL_ALGEBRAIC_CURVE_KERNEL_2_H

#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_Tuple.hpp>
#include <Teuchos_VerboseObject.hpp>
#include <Teuchos_ParameterList.hpp>

#include <Tpetra_DefaultPlatform.hpp>
#include <Tpetra_Map.hpp>
#include <Tpetra_MultiVector.hpp>
#include <Tpetra_CrsMatrix.hpp>

#include "Amesos2.hpp"
#include "Amesos2_Version.hpp"


int main(int argc, char *argv[]) {
  typedef double Scalar;
  typedef Teuchos::ScalarTraits<Scalar>::magnitudeType Magnitude;

  typedef double Scalar;
  typedef int LO;
  typedef int GO;

  typedef Tpetra::CrsMatrix<Scalar,LO,GO> MAT;
  typedef Tpetra::MultiVector<Scalar,LO,GO> MV;

  using Tpetra::global_size_t;
  using Teuchos::tuple;
  using Teuchos::RCP;
  using Teuchos::rcp;

  Teuchos::GlobalMPISession mpiSession(&argc,&argv);
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::DefaultPlatform::getDefaultPlatform().getComm();
  size_t myRank = comm->getRank();

  RCP<Teuchos::FancyOStream> fos = Teuchos::fancyOStream(Teuchos::rcpFromRef(std::cout));
  if( myRank == 0 ) *fos << Amesos2::version() << std::endl << std::endl;

  // create a Map
  global_size_t nrows = 6;
  RCP<Tpetra::Map<LO,GO> > map = rcp( new Tpetra::Map<LO,GO>(nrows,0,comm) );
  RCP<MAT> A = rcp( new MAT(map,3) ); // max of three entries in a row

  /*
   * We will solve a system with a known solution, for which we will be using
   * the following matrix:
   *
   * [ [ 7,  0,  -3, 0, -1,  0 ]
   *   [ 2,  8,  0,  0,  0,  0 ]
   *   [ 0,  0,  1,  0,  0,  0 ]
   *   [-3,  0,  0,  5,  0,  0 ]
   *   [ 0, -1,  0,  0,  4,  0 ]
   *   [ 0,  0,  0, -2,  0,  6 ] ]
   *
   */
  // Construct matrix
  if( myRank == 0 ){
    A->insertGlobalValues(0,tuple<GO>(0,2,4),tuple<Scalar>(7,-3,-1));
    A->insertGlobalValues(1,tuple<GO>(0,1),tuple<Scalar>(2,8));
    A->insertGlobalValues(2,tuple<GO>(2),tuple<Scalar>(1));
    A->insertGlobalValues(3,tuple<GO>(0,3),tuple<Scalar>(-3,5));
    A->insertGlobalValues(4,tuple<GO>(1,4),tuple<Scalar>(-1,4));
    A->insertGlobalValues(5,tuple<GO>(3,5),tuple<Scalar>(-2,6));
  }
  A->fillComplete();

  // Create random X
  const size_t numVectors = 1;
  RCP<MV> X = rcp(new MV(map,numVectors));
  X->randomize();

  /* Create B
   *
   * Use RHS:
   *
   *  [[-7]
   *   [18]
   *   [ 3]
   *   [17]
   *   [18]
   *   [28]]
   */
  RCP<MV> B = rcp(new MV(map,numVectors));
  int data[6] = {-7,18,3,17,18,28};
  for( int i = 0; i < 6; ++i ){
    if( B->getMap()->isNodeGlobalElement(i) ){
      B->replaceGlobalValue(i,0,data[i]);
    }
  }

  // Check first whether SuperLU is supported
  if( Amesos2::query("SuperLU_DIST") ){
  
    // Constructor from Factory
    RCP<Amesos2::Solver<MAT,MV> > solver = Amesos2::create<MAT,MV>("SuperLU_DIST", A, X, B);

    solver->symbolicFactorization();
    solver->numericFactorization();
    solver->solve();

    /* Print the solution
     * 
     * Should be:
     *
     *  [[1]
     *   [2]
     *   [3]
     *   [4]
     *   [5]
     *   [6]]
     */
    X->describe(*fos,Teuchos::VERB_EXTREME);
  } else {
    *fos << "SuperLU solver not enabled.  Exiting..." << std::endl;
  }
}

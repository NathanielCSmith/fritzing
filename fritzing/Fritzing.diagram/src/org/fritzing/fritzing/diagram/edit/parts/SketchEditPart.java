/*
 * (c) Fachhochschule Potsdam
 */
package org.fritzing.fritzing.diagram.edit.parts;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.draw2d.ConnectionAnchor;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.XYAnchor;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PrecisionPoint;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.gef.ConnectionEditPart;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.LayerConstants;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.UnexecutableCommand;
import org.eclipse.gef.editparts.GridLayer;
import org.eclipse.gef.requests.CreateConnectionRequest;
import org.eclipse.gmf.runtime.diagram.core.util.ViewUtil;
import org.eclipse.gmf.runtime.diagram.ui.editparts.DiagramEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.INodeEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.ContainerNodeEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.GraphicalNodeEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateConnectionViewRequest;
import org.eclipse.gmf.runtime.diagram.ui.requests.RequestConstants;
import org.eclipse.gmf.runtime.draw2d.ui.figures.BaseSlidableAnchor;
import org.eclipse.gmf.runtime.gef.ui.figures.SlidableAnchor;
import org.eclipse.gmf.runtime.notation.Edge;
import org.eclipse.gmf.runtime.notation.NotationPackage;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.swt.graphics.Color;
import org.fritzing.fritzing.diagram.edit.policies.SketchCanonicalEditPolicy;
import org.fritzing.fritzing.diagram.edit.policies.SketchItemSemanticEditPolicy;
import org.fritzing.fritzing.diagram.part.FritzingLinkDescriptor;

/**
 * @generated NOT
 */
public class SketchEditPart extends DiagramEditPart implements INodeEditPart {

	/**
	 * @generated
	 */
	public final static String MODEL_ID = "FritzingPhysical"; //$NON-NLS-1$

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 1000;

	/**
	 * @generated
	 */
	public SketchEditPart(View view) {
		super(view);
	}

	/**
	 * @generated NOT
	 */
	protected void createDefaultEditPolicies() {
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new SketchItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new SketchCanonicalEditPolicy());

		// kills the drag-wire from terminal to sketch popup
		//		installEditPolicy(EditPolicy.GRAPHICAL_NODE_ROLE, new NoPopupContainerNodeEditPolicy());

		// allows a leg to be attached to a sketch
		installEditPolicy(EditPolicy.GRAPHICAL_NODE_ROLE,
				new GraphicalNodeEditPolicy());

		// POPUP_BAR and CONNECTOR_HANDLES are disabled by default in
		// preferences
	}

	// implements INodeEditPart
	public ConnectionAnchor getSourceConnectionAnchor(
			ConnectionEditPart connection) {
		// sketch should never be the source
		return null;
	}

	// implements INodeEditPart
	public ConnectionAnchor getTargetConnectionAnchor(
			ConnectionEditPart connection) {
		EditPart part = connection.getSource();
		if (part instanceof Terminal2EditPart) {
			// if it's null, create one here
			return ((Terminal2EditPart) part).getLegConnectionAnchor();
		}
		return null;
	}

	// implements INodeEditPart
	public ConnectionAnchor getSourceConnectionAnchor(Request request) {
		// sketch should never be the source
		return null;
	}

	// implements INodeEditPart
	public ConnectionAnchor getTargetConnectionAnchor(Request request) {
		if (request instanceof CreateConnectionViewRequest) {
			EditPart part = ((CreateConnectionViewRequest) request)
					.getSourceEditPart();
			if (part instanceof Terminal2EditPart) {
				Point p = ((Terminal2EditPart) part).getLegTargetPosition();
				TerminalAnchor ta = new TerminalAnchor(p,
						(Terminal2EditPart) part);
				((Terminal2EditPart) part).setLegConnectionAnchor(ta);
				return ta;
			}
		}

		return null;
	}

	// implements INodeEditPart
	public String mapConnectionAnchorToTerminal(ConnectionAnchor c) {
		if (c instanceof TerminalAnchor) {
			return ((TerminalAnchor) c).getTerminal();
		}

		return "";
	}

	// implements INodeEditPart
	public ConnectionAnchor mapTerminalToConnectionAnchor(String terminal) {
		// sketch should never be the source
		return null;
	}

	// implements INodeEditPart
	public boolean canAttachNote() {
		return false;
	}

	// need to catch the notification when the model adds a leg
	protected void handleNotificationEvent(Notification notification) {
		Object feature = notification.getFeature();
		if (NotationPackage.eINSTANCE.getView_TargetEdges().equals(feature))
			refreshTargetConnections();
		else
			super.handleNotificationEvent(notification);
	}

	// the default returns an empty list
	protected List getModelTargetConnections() {
		View view = (View) getModel();
		// if (!view.eIsSet(NotationPackage.Literals.VIEW__TARGET_EDGES))
		//    return Collections.EMPTY_LIST;
		List targetConnections = new ArrayList();
		Iterator iter = view.getTargetEdges().iterator();
		while (iter.hasNext()) {
			Edge edge = (Edge) iter.next();
			View source = edge.getSource();
			//            if (edge.isVisible() && isVisible(source)){
			targetConnections.add(edge);
			//           }
		}
		return targetConnections;
	}

	public class NoPopupContainerNodeEditPolicy extends ContainerNodeEditPolicy {
		public Command getCommand(Request request) {

			if (RequestConstants.REQ_CONNECTION_END.equals(request.getType())
					&& request instanceof CreateConnectionRequest) {
				// don't popup a menu if the user drags out a wire from a terminal and drops it on the sketch
				// but allow the request if it's a leg!

				if (request instanceof CreateConnectionViewRequest) {
					CreateConnectionViewRequest.ConnectionViewDescriptor cvd = ((CreateConnectionViewRequest) request)
							.getConnectionViewDescriptor();
					IAdaptable adapter = cvd.getElementAdapter();
					if (adapter instanceof FritzingLinkDescriptor) {
						int id = ((FritzingLinkDescriptor) adapter)
								.getVisualID();
					}
				}

				//return null;
			}

			return super.getCommand(request);
		}

	}

	public class TerminalAnchor extends XYAnchor {
		protected Terminal2EditPart terminal;

		public TerminalAnchor(Point p, Terminal2EditPart terminal) {
			super(p);
			this.terminal = terminal;
		}

		public String getTerminal() {
			Point p = this.getReferencePoint();
			return new String("(" + ((float) p.x) + "," + ((float) p.y) + ")");
		}

		public Point getLocation(Point reference) {
			Point p = this.getReferencePoint();
			return new Point(reference.x + p.x, reference.y + p.y);
		}

	}
}
